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

#define _DEFAULT_SOURCE

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

static int32_t _workers_max;
static int32_t _worker_id;
static struct shared_t *_shared;

// statistics
#define STATS_FREQUENCY 10000

uint64_t calls;
uint64_t cuts_prop;
uint64_t cuts_bound;
uint64_t cuts_obj;
uint64_t cut_depth;
size_t depth_min;
size_t depth_max;
extern size_t alloc_max;

void stats_init(void) {
  calls = 0;
  cuts_prop = 0;
  cuts_bound = 0;
  cuts_obj = 0;
  cut_depth = 0;
  depth_min = SIZE_MAX;
  depth_max = 0;
  alloc_max = 0;
}

void print_stats(FILE *file) {
  fprintf(file, "#%d: CALLS: %ld, CUTS: %ld/%ld, BOUND: %ld, DEPTH: %ld/%ld, AVG DEPTH: %f, MEMORY: %ld, SOLUTIONS: %ld\n",
          _worker_id,
          calls, cuts_prop, cuts_obj, cuts_bound,
          depth_min, depth_max,
          (double)cut_depth / (cuts_prop + cuts_obj + cuts_bound),
          alloc_max, shared()->solutions);
  fflush(file);
  depth_min = SIZE_MAX;
  depth_max = 0;
}

void shared_init(int32_t workers_max) {
  _workers_max = workers_max;
  _shared = mmap(NULL, sizeof(struct shared_t),
                 PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  sema_init(&shared()->semaphore);
  shared()->workers = 1;
  shared()->workers_id = 1;
  _worker_id = 1;
}

struct shared_t *shared(void) {
  return _shared;
}

pid_t worker_spawn(struct env_t *env, size_t depth) {

  if (env[depth+1].key == NULL) {
    // do not fork at last level
    return -1;
  }

  if (!is_value(*env[depth].val) && shared()->workers < _workers_max) {

    sema_wait(&shared()->semaphore);
    // avoid race condition
    if (shared()->workers >= _workers_max) {
      sema_post(&shared()->semaphore);
      return -1;
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
      // reset stats for child
      stats_init();
      depth_min = depth;
      depth_max = depth;
    } else {
      // parent searches the lower half
      struct val_t v = (lo == mid) ? VALUE(lo) : INTERVAL(lo, mid);
      bind(env[depth].val, v);
    }
    return pid;
  }

  return -1;
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

void worker_die(pid_t pid) {
  if (pid == 0) {
    // throttle the reation of new workers
    if (_workers_max > 1 && shared()->workers == _workers_max) {
      usleep(100000);
    }

    sema_wait(&shared()->semaphore);
    shared()->workers--;
    sema_post(&shared()->semaphore);

    await_children();

    if (calls > 0) {
      print_stats(stdout);
    }

    exit(EXIT_SUCCESS);
  }
}

void swap_env(struct env_t *env, size_t depth1, size_t depth2) {
  if (env[depth1].key != NULL && env[depth2].key != NULL) {
    struct env_t t = env[depth2];
    env[depth2] = env[depth1];
    env[depth1] = t;
  }
}

bool solve_value(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth, struct val_t val) {
  bool failed = true;

  // mark how much memory is allocated
  void *alloc_marker = alloc(0);

  // bind variable
  size_t bind_depth = bind(env[depth].val, val);

  bool better = objective_better(obj);

  // continue if better objective value is possible
  if (better) {

    // optimize objective function
    struct constr_t *new_obj = objective_optimize(obj);

    // continue if objective function is feasible
    if (new_obj != NULL) {

      // optimize constraints
      struct constr_t *prop = propagate(constr, VALUE(1));

      // continue if still feasible
      if (prop != NULL) {

        struct constr_t *norm = normalize(prop);

        // solve recursively
        solve(env, new_obj, norm, depth+1);
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

  // unbind variable again
  unbind(bind_depth);

  // free up memory
  dealloc(alloc_marker);

  return failed;
}

void solve_variable(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth) {
  struct val_t *var = env[depth].val;
  switch (var->type) {
  case VAL_INTERVAL: {
    bool failed = true;
    restart:;
    domain_t lo = get_lo(*var);
    domain_t hi = get_hi(*var);
    uint64_t s = shared()->solutions;

    for (domain_t i = 0; i <= hi - lo; i++) {
      // search from the edges of the interval
      domain_t v = (i & 1) ? hi - (i >> 1) : lo + (i >> 1);

      // solve for particular value of variable
      failed &= solve_value(env, obj, constr, depth, VALUE(v));

      // stop searching if looking just for any solution
      if (objective() == OBJ_ANY && shared()->solutions > 0) {
        break;
      }

      // some new solutions were found
      if (s != shared()->solutions) {
        // optimize objective function
        obj = objective_optimize(obj);
        // abort if objective function has become infeasible
        if (obj == NULL) {
          break;
        }
        // restart if bounds for current variable have changed
        if (lo != get_lo(*var) || hi != get_hi(*var)) {
          goto restart;
        }
      }
    }

    // swap failed depth forward
    if (strategy_prefer_failing() && !failed) {
      swap_env(env, depth, depth+1);
    }
    break;
  }
  case VAL_VALUE:
    solve(env, obj, constr, depth+1);
    if (strategy_prefer_failing()) {
      swap_env(env, depth, depth+1);
    }
    break;
  default:
    print_error(ERROR_MSG_INVALID_VARIABLE_TYPE, var->type);
  }
}

void solve(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth) {
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

  if (env[depth].key != NULL) {
    strategy_pick_var(env, depth);
    pid_t pid = worker_spawn(env, depth);
    solve_variable(env, obj, constr, depth);
    worker_die(pid);
  } else {
    struct val_t feasible = eval(constr);
    if (is_true(feasible)) {
      sema_wait(&shared()->semaphore);

      if (!(objective() == OBJ_ANY && shared()->solutions > 0)
          && objective_better(obj)) {

        objective_update(eval(obj));
        fprintf(stderr, "#%d: ", _worker_id);
        print_solution(stderr, env);
        shared()->solutions++;
      }

      sema_post(&shared()->semaphore);
    }
  }

  if (depth == 0) {
    worker_die(0);
  }
}

