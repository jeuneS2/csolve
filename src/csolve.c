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
#include "csolve.h"

// statistics
#define STATS_FREQUENCY 10000

uint64_t calls = 0;
uint64_t cuts_prop = 0;
uint64_t cuts_bound = 0;
uint64_t cuts_obj = 0;
uint64_t cut_depth = 0;
uint64_t solutions = 0;
extern size_t alloc_max;

void print_stats(FILE *file) {
  fprintf(file, "CALLS: %ld, CUTS: %ld/%ld, BOUND: %ld, AVG DEPTH: %f, MEMORY: %ld, SOLUTIONS: %ld\n",
          calls, cuts_prop, cuts_obj, cuts_bound,
          (double)cut_depth / (cuts_prop + cuts_obj + cuts_bound),
          alloc_max, solutions);
}

void solve_value(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth, struct val_t val) {
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
      struct constr_t *norm = normalize(constr);
      struct constr_t *prop = propagate(norm, VALUE(1));

      // continue if still feasible
      if (prop != NULL) {

        // solve recursively
        solve(env, new_obj, prop, depth+1);

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
}

void solve_variable(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth) {
  struct val_t *var = env[depth].val;
  switch (var->type) {
  case VAL_INTERVAL: {
    restart:;
    domain_t lo = get_lo(*var);
    domain_t hi = get_hi(*var);
    uint64_t s = solutions;

    for (domain_t i = 0; i <= hi - lo; i++) {
      // search from the edges of the interval
      domain_t v = (i & 1) ? hi - (i >> 1) : lo + (i >> 1);

      // solve for particular value of variable
      solve_value(env, obj, constr, depth, VALUE(v));

      // stop searching if looking just for any solution
      if (objective() == OBJ_ANY && solutions > 0) {
        break;
      }

      // some new solutions were found
      if (s != solutions) {
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
    break;
  }
  case VAL_VALUE:
    solve(env, obj, constr, depth+1);
    break;
  default:
    print_error(ERROR_MSG_INVALID_VARIABLE_TYPE, var->type);
  }
}

void solve(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth) {
  calls++;
  if (calls % STATS_FREQUENCY == 0) {
    print_stats(stdout);
  }

  if (env[depth].key != NULL) {
    solve_variable(env, obj, constr, depth);
  } else {
    struct val_t feasible = eval(constr);
    if (is_true(feasible) && objective_better(obj)) {
      objective_update(eval(obj));
      print_solution(stdout, env);
      solutions++;
    }
  }
}

