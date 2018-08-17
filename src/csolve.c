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

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

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
static size_t _worker_min_depth;
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
  stat_reset_depth_min();
  stat_reset_depth_max();
}

static void update_stats(size_t depth) {
  stat_min_depth_min(depth);
  stat_max_depth_max(depth);
  stat_inc_calls();
  if (stat_get_calls() % STATS_FREQUENCY == 0) {
    print_stats(stdout);
  }
}

// calculate Luby sequence using algorithm by Knuth
uint64_t fail_threshold_next(void) {
  uint64_t threshold = _fail_threshold;
  if ((_fail_threshold_counter & -_fail_threshold_counter) == _fail_threshold) {
    _fail_threshold_counter++;
    _fail_threshold = 1;
  } else {
    _fail_threshold <<= 1;
  }
  return threshold;
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
  _worker_min_depth = 0;
}

struct shared_t *shared(void) {
  return _shared;
}

void worker_spawn(struct env_t *env, size_t depth) {

  if (!is_value(*env[depth].val) && shared()->workers < _workers_max) {

    sema_wait(&shared()->semaphore);
    // avoid race condition
    if (shared()->workers >= _workers_max) {
      sema_post(&shared()->semaphore);
      return;
    }
    ++shared()->workers;
    int32_t id = ++shared()->workers_id;
    sema_post(&shared()->semaphore);

    domain_t lo = get_lo(*env[depth].val);
    domain_t hi = get_hi(*env[depth].val);
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
      bind(env[depth].val, v);
      _worker_min_depth = depth;
      // reset stats for child
      stats_init();
      stat_set_depth_min(depth);
      stat_set_depth_max(depth);
    } else {
      // parent searches the lower half
      struct val_t v = (lo == mid) ? VALUE(lo) : INTERVAL(lo, mid);
      bind(env[depth].val, v);
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
}

// algorithm helper functions
void swap_env(struct env_t *env, size_t depth1, size_t depth2) {
  if (env[depth1].key != NULL && env[depth2].key != NULL && depth1 != depth2) {
    struct env_t t = env[depth2];
    env[depth2].key = env[depth1].key;
    env[depth2].val = env[depth1].val;
    env[depth2].fails = env[depth1].fails;
    env[depth1].key = t.key;
    env[depth1].val = t.val;
    env[depth1].fails = t.fails;
  }
}

static inline bool found_any(void) {
  return objective() == OBJ_ANY && shared()->solutions > 0;
}

static inline bool is_restartable(void) {
  return objective() == OBJ_ANY && strategy_restart_frequency() > 0;
}

static bool update_solution(struct env_t *env, struct constr_t *constr) {
  bool updated = false;

  if (is_true(eval(constr))) {
    sema_wait(&shared()->semaphore);

    if (!found_any() && objective_better()) {

      objective_update_best();
      fprintf(stderr, "#%d: ", _worker_id);
      print_solution(stderr, env);
      shared()->solutions++;

      updated = true;
    }

    sema_post(&shared()->semaphore);
  }

  return updated;
}

static bool check_assignment(struct env_t *env, size_t depth) {
  bool failed = true;

  // propagate values
  struct constr_t *prop = propagate(env[depth].step->constr, VALUE(1));
  // check if still feasible
  if (prop != NULL) {
    // normalize constraints
    env[depth+1].step->constr = normal(prop);
    // proceed with next variable
    failed = false;
  } else {
    stat_inc_cuts();
    stat_add_cut_depth(depth);
  }

  cache_clean();
  
  return failed;
}



static bool check_restart(void) {
  // check whether search should restart
  if (is_restartable()) {
    _fail_count++;

    if (_fail_count > _fail_threshold * strategy_restart_frequency()) {
      _fail_count = 0;
      _fail_threshold = fail_threshold_next();
      stat_inc_restarts();
      return true;
    }
  }
  return false;
}

static void step_activate(struct env_t *env, size_t depth) {
  // set up iteration
  env[depth].step->active = true;
  env[depth].step->bounds = *env[depth].val;
  env[depth].step->iter = 0;
  env[depth].step->seed = is_restartable() ? rand() : 0;
}

static void step_deactivate(struct env_t *env, size_t depth) {
  env[depth].step->active = false;
}

static void step_enter(struct env_t *env, size_t depth, domain_t val) {
  // mark how much memory is allocated
  env[depth].step->alloc_marker = alloc(0);
  // mark patching depth
  env[depth].step->patch_depth = patch(NULL, (struct wand_expr_t){ NULL, 0 });
  // bind variable
  env[depth].step->bind_depth = bind(env[depth].val, VALUE(val));
}

static void step_leave(struct env_t *env, size_t depth) {
  // unbind variable
  unbind(env[depth].step->bind_depth);
  // unbind wide-ands
  unpatch(env[depth].step->patch_depth);
  // free up memory
  dealloc(env[depth].step->alloc_marker);
}

static void step_next(struct env_t *env, size_t depth) {
  // continue with next iteration
  env[depth].step->iter++;
}

static bool step_check(struct env_t *env, size_t depth) {
  // check if search space for this variable is exhausted
  udomain_t i = env[depth].step->iter;
  domain_t lo = get_lo(env[depth].step->bounds);
  domain_t hi = get_hi(env[depth].step->bounds);
  return i <= (udomain_t)(hi - lo);
}

static domain_t step_val(struct env_t *env, size_t depth) {
  // search from the edges of the interval
  udomain_t i = env[depth].step->iter;
  udomain_t s = env[depth].step->seed;
  domain_t lo = get_lo(env[depth].step->bounds);
  domain_t hi = get_hi(env[depth].step->bounds);
  return ((i ^ s) & 1) ? hi - (i >> 1) : lo + (i >> 1);
}

static void unwind(struct env_t *env, size_t depth, size_t stop) {
  // unwind search steps up to a specified level
  for (size_t i = depth; i != (stop-1); --i) {
    step_deactivate(env, i);
    step_leave(env, i);
  }
}

// backtrack by one level
#define BACKTRACK()                             \
  if (depth != 0) {                             \
    depth--;                                    \
    continue;                                   \
  } else {                                      \
    break;                                      \
  }

// restart the search
#define RESTART()                               \
  {                                             \
    unwind(env, depth, _worker_min_depth);      \
    depth = _worker_min_depth;                  \
    continue;                                   \
  }

// exit the search
#define EXIT()                                  \
  {                                             \
    break;                                      \
  }

// search algorithm core
void solve(struct env_t *env, struct constr_t *constr) {
  size_t depth = 0;
  env[depth].step->constr = constr;

  while (true) {
    if (depth < _worker_min_depth) {
      EXIT();
    }

    // stop searching if a solution is found and we just need any solution
    if (found_any()) {
      EXIT();
    }

    // check if a better feasible solution is reached
    if (env[depth].key == NULL) {
      bool update = update_solution(env, env[depth].step->constr);
      if (update) {
        depth--;
        RESTART();
      } else {
        BACKTRACK();
      }
    }

    if (!env[depth].step->active) {
      // not active yet, so we can pick a variable
      strategy_pick_var(env, depth);
      // spawn a worker if possible
      worker_spawn(env, depth);
      step_activate(env, depth);
    } else {
      // continue iteration
      step_leave(env, depth);
      step_next(env, depth);
    }

    // check if values for variable are exhausted
    if (!step_check(env, depth)) {
      step_deactivate(env, depth);
      BACKTRACK();
    }

    // try next value
    step_enter(env, depth, step_val(env, depth));

    // update objective value
    objective_update_val();

    update_stats(depth);

    // decide whether to move to next variable, stay at current one, or restart
    bool failed = check_assignment(env, depth);
    if (!failed) {
      depth++;
    } else {
      env[depth].fails++;
      if (check_restart()) {
        RESTART();
      }
    }
  }

  // wait for children and die
  worker_die();
}

