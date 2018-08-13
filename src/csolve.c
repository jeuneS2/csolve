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

uint64_t calls;
uint64_t cuts_prop;
uint64_t cuts_bound;
uint64_t cuts_obj;
uint64_t cut_depth;
uint64_t restarts;
size_t depth_min;
size_t depth_max;
extern size_t alloc_max;

void stats_init(void) {
  calls = 0;
  cuts_prop = 0;
  cuts_bound = 0;
  cuts_obj = 0;
  cut_depth = 0;
  restarts = 0;
  depth_min = SIZE_MAX;
  depth_max = 0;
  alloc_max = 0;
}

void stats_update(size_t depth) {
  if (depth < depth_min) {
    depth_min = depth;
  }
  if (depth > depth_max) {
    depth_max = depth;
  }
  calls++;
  if (calls % STATS_FREQUENCY == 0) {
    print_stats(stdout);
  }
}

void print_stats(FILE *file) {
  fprintf(file, "#%d: CALLS: %lu, CUTS: %lu/%lu, BOUND: %lu, RESTARTS: %lu, DEPTH: %lu/%lu, AVG DEPTH: %f, MEMORY: %lu, SOLUTIONS: %lu\n",
          _worker_id,
          calls, cuts_prop, cuts_obj, cuts_bound,
          restarts,
          depth_min, depth_max,
          (double)cut_depth / (cuts_prop + cuts_obj + cuts_bound),
          alloc_max, shared()->solutions);
  fflush(file);
  depth_min = SIZE_MAX;
  depth_max = 0;
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
      depth_min = depth;
      depth_max = depth;
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

  if (calls > 0) {
    print_stats(stdout);
  }
}

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

static bool update_solution(struct env_t *env, struct constr_t *obj, struct constr_t *constr) {
  bool updated = false;

  if (is_true(eval(constr))) {
    sema_wait(&shared()->semaphore);

    if (!(objective() == OBJ_ANY && shared()->solutions > 0)
        && objective_better(obj)) {

      objective_update(eval(obj));
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

  // check if better objective value is possible
  bool better = objective_better(env[depth].step->obj);
  if (better) {
    // optimize objective function
    env[depth+1].step->obj = objective_optimize(env[depth].step->obj);
    // check if objective function is feasible
    if (env[depth+1].step->obj != NULL) {
      // optimize constraints
      struct constr_t *prop = propagate(env[depth].step->constr, VALUE(1));
      // check if still feasible
      if (prop != NULL) {
        env[depth+1].step->constr = normalize(prop);
        // proceed with next variable
        failed = false;
      } else {
        cuts_prop++;
        cut_depth += depth;
      }
    } else {
      cuts_obj++;
      cut_depth += depth;
    }
  } else {
    cuts_bound++;
    cut_depth += depth;
  }

  cache_clean();
  
  return failed;
}

static bool check_restart(void) {
  // check whether search should restart
  if (strategy_restart_frequency() > 0 && objective() == OBJ_ANY) {
    _fail_count++;

    if (_fail_count > _fail_threshold * strategy_restart_frequency()) {
      _fail_count = 0;
      _fail_threshold = fail_threshold_next();
      restarts++;
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
  env[depth].step->seed = strategy_restart_frequency() > 0 ? rand() : 0;
}

static void step_deactivate(struct env_t *env, size_t depth) {
  env[depth].step->active = false;
}

static void step_enter(struct env_t *env, size_t depth, domain_t val) {
  // mark how much memory is allocated
  env[depth].step->alloc_marker = alloc(0);
  // bind variable
  env[depth].step->bind_depth = bind(env[depth].val, VALUE(val));
}

static void step_leave(struct env_t *env, size_t depth) {
  // unbind variable
  unbind(env[depth].step->bind_depth);
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
    unwind(env, depth, 0);                      \
    depth = 0;                                  \
    continue;                                   \
  }

// exit the search
#define EXIT()                                  \
  {                                             \
    break;                                      \
  }

void solve(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t x) {
  size_t depth = 0;
  env[depth].step->obj = obj;
  env[depth].step->constr = constr;

  while (true) {
    if (depth < _worker_min_depth) {
      EXIT();
    }

    // stop searching if a solution is found and we just need any solution
    if (objective() == OBJ_ANY && shared()->solutions > 0) {
      EXIT();
    }

    // check if a better feasible solution is reached
    if (env[depth].key == NULL) {
      bool updated = update_solution(env, env[depth].step->obj, env[depth].step->constr);
      if (updated) {
        if (objective() == OBJ_ANY) {
          EXIT();
        } else {
          depth--;
          RESTART();
        }
      }
      BACKTRACK();
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

    stats_update(depth);

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

