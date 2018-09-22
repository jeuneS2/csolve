/* Copyright 2018 Wolfgang Puffitsch

This file is part of CSolve.

CSolve is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

CSolve is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with CSolve.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "csolve.h"

// parallelizing data
static uint32_t _workers_max;
static uint32_t _worker_id;
static size_t _worker_min_level;
static struct shared_t *_shared;

// restarting data
static uint32_t _fail_count = 0;
static uint64_t _fail_threshold = 1;
static uint64_t _fail_threshold_counter = 1;

// statistics
#define STATS_FREQUENCY 10000

static void print_stats(FILE *file) {
  fprintf(file, "#%d: ", _worker_id);
  stats_print(file);
  fprintf(file, ", SOLUTIONS: %lu\n", shared()->solutions);
  fflush(file);
  stat_reset_level_min();
  stat_reset_level_max();
}

static void update_stats(size_t level) {
  stat_min_level_min(level);
  stat_max_level_max(level);
  stat_inc_calls();
  if (stat_get_calls() % STATS_FREQUENCY == 0) {
    print_stats(stdout);
  }
}

// calculate Luby sequence using algorithm by Knuth
void fail_threshold_next(void) {
  if ((_fail_threshold_counter & -_fail_threshold_counter) == _fail_threshold) {
    _fail_threshold_counter++;
    _fail_threshold = 1;
  } else {
    _fail_threshold <<= 1;
  }
}

void shared_init(int32_t workers_max) {
  _workers_max = workers_max;
  _shared = (struct shared_t *)mmap(NULL, sizeof(struct shared_t),
                                    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,
                                    -1, 0);
  sema_init(&shared()->semaphore);
  shared()->workers = 1;
  shared()->workers_id = 1;
  _worker_id = 1;
  _worker_min_level = 0;
}

struct shared_t *shared(void) {
  return _shared;
}

void worker_spawn(struct env_t *var, size_t level) {

  if (!is_value(var->val->constr.term.val) && shared()->workers < _workers_max) {

    sema_wait(&shared()->semaphore);
    // avoid race condition
    if (shared()->workers >= _workers_max) {
      sema_post(&shared()->semaphore);
      return;
    }
    ++shared()->workers;
    int32_t id = ++shared()->workers_id;
    sema_post(&shared()->semaphore);

    domain_t lo = get_lo(var->val->constr.term.val);
    domain_t hi = get_hi(var->val->constr.term.val);
    domain_t mid = (lo + hi) / 2;

    // pick up any children that may have finished
    int status;
    waitpid(-1, &status, WNOHANG);

    pid_t pid = fork();
    if (pid == -1) {
      print_error("%s", strerror(errno));
      exit(EXIT_FAILURE);
    } else if (pid == 0) {
      // child gets a new id and searches the upper half
      _worker_id = id;
      struct val_t v = (mid+1 == hi) ? VALUE(hi) : INTERVAL(mid+1, hi);
      bind_level_set(level);
      bind(var, v, NULL);
      _worker_min_level = level;
      // reset stats for child
      stats_init();
      stat_set_level_min(level);
      stat_set_level_max(level);
    } else {
      // parent searches the lower half
      struct val_t v = (lo == mid) ? VALUE(lo) : INTERVAL(lo, mid);
      bind_level_set(level);
      bind(var, v, NULL);
    }
  }
}

void await_children(void) {
  for (;;) {
    int status;
    pid_t pid = wait(&status);
    if (pid == -1 && errno == ECHILD) {
      break;
    }
  }
}

void worker_die(void) {
  sema_wait(&shared()->semaphore);
  shared()->workers--;
  sema_post(&shared()->semaphore);

  await_children();

  if (stat_get_calls() > 0) {
    print_stats(stdout);
  }

  if (_worker_id == 1 && shared()->solutions == 0) {
    fprintf(stdout, "NO SOLUTION FOUND\n");
  }
}

// algorithm helper functions
static inline bool found_any(void) {
  return objective() == OBJ_ANY && shared()->solutions > 0;
}

static inline bool is_restartable(void) {
  return objective() == OBJ_ANY && strategy_restart_frequency() > 0;
}

static bool update_solution(size_t size, struct env_t *env, struct constr_t *constr) {
  bool updated = false;

  if (is_true(constr->type->eval(constr))) {
    sema_wait(&shared()->semaphore);

    if (!found_any() && objective_better()) {

      objective_update_best();
      fprintf(stderr, "#%d: ", _worker_id);
      print_solution(stdout, size, env);
      shared()->solutions++;

      updated = true;
    }

    sema_post(&shared()->semaphore);
  }

  return updated;
}

static bool check_assignment(struct env_t *var, size_t level) {
  // propagate values
  bool failed =
    propagate_clauses(&var->clauses) == PROP_ERROR ||
    (objective_val() != NULL && objective_val()->constr.term.env != NULL &&
     propagate_clauses(&objective_val()->constr.term.env->clauses) == PROP_ERROR);

  // update statistics if propagation failed
  if (failed) {
    stat_inc_cuts();
    stat_add_cut_level(level);
  }

  return failed;
}

static bool check_restart(void) {
  // check whether search should restart
  if (is_restartable()) {
    _fail_count++;

    if (_fail_count > _fail_threshold * strategy_restart_frequency()) {
      _fail_count = 0;
      fail_threshold_next();
      stat_inc_restarts();
      return true;
    }
  }
  return false;
}

static void step_activate(struct step_t *step, struct env_t *var) {
  // set up iteration
  step->active = true;
  step->var = var;
  step->bounds = var->val->constr.term.val;
  step->iter = 0;
  step->seed = is_restartable() ? rand() : 0;
}

static void step_deactivate(struct step_t *step){
  strategy_var_order_push(step->var);
  step->active = false;
}

static void step_enter(struct step_t *step, domain_t val) {
  // mark how much memory is allocated
  step->alloc_marker = alloc(0);
  // mark patching depth
  step->patch_depth = patch(NULL, NULL);
  // bind variable
  step->bind_depth = bind_depth();
  if (!is_const(step->var->val)) {
    bind(step->var, VALUE(val), NULL);
  }
}

static void step_leave(struct step_t *step) {
  // unbind variable
  unbind(step->bind_depth);
  // unbind wide-ands
  unpatch(step->patch_depth);
  // free up memory
  dealloc(step->alloc_marker);
}

static void step_next(struct step_t *step) {
  // continue with next iteration
  step->iter++;
}

static bool step_check(struct step_t *step) {
  // check if search space for this variable is exhausted
  udomain_t i = step->iter;
  domain_t lo = get_lo(step->bounds);
  domain_t hi = get_hi(step->bounds);
  return i <= (udomain_t)(hi - lo);
}

static domain_t step_val(struct step_t *step) {
  // search from the edges of the interval
  udomain_t i = step->iter;
  udomain_t s = step->seed;
  domain_t lo = get_lo(step->bounds);
  domain_t hi = get_hi(step->bounds);
  return ((i ^ s) & 1) ? hi - (i >> 1) : lo + (i >> 1);
}

static void unwind(struct step_t *steps, size_t level, size_t stop) {
  // unwind search steps up to a specified level
  for (size_t i = level; i != (stop-1); --i) {
    step_leave(&steps[i]);
    step_deactivate(&steps[i]);
  }
}

// backtrack by one level
#define BACKTRACK()                             \
  if (level != 0) {                             \
    level--;                                    \
    continue;                                   \
  } else {                                      \
    break;                                      \
  }

// restart the search
#define RESTART()                               \
  {                                             \
    unwind(steps, level, _worker_min_level);    \
    level = _worker_min_level;                  \
    continue;                                   \
  }

// exit the search
#define EXIT()                                  \
  {                                             \
    break;                                      \
  }

// search algorithm core
void solve(size_t size, struct env_t *env, struct constr_t *constr) {

  // allocate data structure for search steps
  struct step_t *steps = (struct step_t *)calloc(size, sizeof(struct step_t));

  size_t level = 0;

  while (true) {
    if (level < _worker_min_level) {
      EXIT();
    }

    // stop searching if a solution is found and we just need any solution
    if (found_any()) {
      EXIT();
    }

    // check if a better feasible solution is reached
    if (level == size) {
      bool update = update_solution(size, env, constr);
      if (update) {
        level--;
        RESTART();
      } else {
        BACKTRACK();
      }
    }

    if (!steps[level].active) {
      // pick a variable
      struct env_t *var = strategy_var_order_pop();
      // spawn a worker if possible
      worker_spawn(var, level);
      step_activate(&steps[level], var);
    } else {
      // continue iteration
      step_leave(&steps[level]);
      step_next(&steps[level]);
    }

    // check if values for variable are exhausted
    if (!step_check(&steps[level])) {
      step_deactivate(&steps[level]);
      BACKTRACK();
    }

    // try next value
    bind_level_set(level);
    step_enter(&steps[level], step_val(&steps[level]));

    // update objective value
    objective_update_val();

    update_stats(level);

    // decide whether to move to next variable, stay at current one, or restart
    bool failed = check_assignment(steps[level].var, level);
    if (!failed) {
      steps[level].var->prio--;
      level++;
    } else {
      steps[level].var->prio++;
      if (check_restart()) {
        RESTART();
      } else if (strategy_create_conflicts()) {
        prop_result_t p = PROP_ERROR;
        if (conflict_level() <= level) {
          unwind(steps, level, level);
        }
        while (p == PROP_ERROR && conflict_level() <= level) {
          unwind(steps, level-1, conflict_level());
          level = conflict_level();
          bind_level_set(level-1);
          p = propagate_clauses(&conflict_var()->clauses);
        }
        continue;
      }
    }
  }

  // release memory again
  free(steps);

  // wait for children and die
  worker_die();
}

