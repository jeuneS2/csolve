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
size_t alloc_max = 0;

void print_stats(FILE *file) {
  fprintf(file, "CALLS: %ld, CUTS: %ld/%ld, BOUND: %ld, AVG DEPTH: %f, MEMORY: %ld, SOLUTIONS: %ld\n",
          calls, cuts_prop, cuts_obj, cuts_bound,
          (double)cut_depth / (cuts_prop + cuts_obj + cuts_bound),
          alloc_max, solutions);
}

// memory allocation functions
#define ALLOC_ALIGNMENT 8
#define ALLOC_STACK_SIZE (64*0x400*0x400)
char alloc_stack[ALLOC_STACK_SIZE];
size_t alloc_stack_pointer = 0;

void *alloc(size_t size) {
  // align size
  size_t sz = (size + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  // allocate memory
  if (alloc_stack_pointer + sz < ALLOC_STACK_SIZE) {
    void *retval = (void *)&alloc_stack[alloc_stack_pointer];
    alloc_stack_pointer += sz;
    if (alloc_stack_pointer > alloc_max) {
      alloc_max = alloc_stack_pointer;
    }
    return retval;
  } else {
    fprintf(stderr, ERROR_MSG_OUT_OF_MEMORY);
    return NULL;
  }
}

void dealloc(void *elem) {
  size_t pointer = (char *)elem - &alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 &&
      pointer >= 0 && pointer <= alloc_stack_pointer) {
    alloc_stack_pointer = pointer;
  } else {
    fprintf(stderr, ERROR_MSG_WRONG_DEALLOC);
  }
}

// functions to bind (and unbind) variables
#define MAX_BINDS 1024
struct binding_t bind_stack[MAX_BINDS];
size_t bind_depth = 0;

size_t bind(struct val_t *loc, const struct val_t val) {
  if (loc != NULL) {
    if (bind_depth < MAX_BINDS) {
      bind_stack[bind_depth].loc = loc;
      bind_stack[bind_depth].val = *loc;
      *loc = val;
      eval_cache_invalidate();
      return bind_depth++;
    } else {
      fprintf(stderr, ERROR_MSG_TOO_MANY_BINDS);
      return MAX_BINDS;
    }
  } else {
    fprintf(stderr, ERROR_MSG_NULL_BIND);
    return MAX_BINDS;
  }
}

void unbind(size_t depth) {
  while (bind_depth > depth) {
    --bind_depth;
    struct val_t *loc = bind_stack[bind_depth].loc;
    *loc = bind_stack[bind_depth].val;
    eval_cache_invalidate();
  }
}

struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l == NULL || r == NULL) {
    return NULL;
  }
  if (l != constr->constr.expr.l || r != constr->constr.expr.r) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.op = constr->constr.expr.op;
    retval->constr.expr.l = l;
    retval->constr.expr.r = r;
    return retval;
  }
  return constr;
}

struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  if (l == NULL) {
    return NULL;
  }
  if (l != constr->constr.expr.l) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.op = constr->constr.expr.op;
    retval->constr.expr.l = l;
    retval->constr.expr.r = NULL;
    return retval;
  }
  return constr;
}

void solve_value(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth, struct val_t val) {
  // mark how much memory is allocated
  void *alloc_marker = alloc(0);

  // bind variable
  size_t bind_depth = bind(env[depth].val, val);

  bool better = objective_better(env, obj);

  // continue if better objective value is possible
  if (better) {

    // optimize objective function
    struct constr_t *new_obj = objective_optimize(env, obj);

    // continue if objective function is feasible
    if (new_obj != NULL) {

      // optimize constraints
      struct constr_t *norm = normalize(env, constr);
      struct constr_t *prop = propagate(env, norm, VALUE(1));

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
        obj = objective_optimize(env, obj);
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
    fprintf(stderr, ERROR_MSG_INVALID_VARIABLE_TYPE, var->type);
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
    struct val_t feasible = eval(env, constr);
    if (is_true(feasible) && objective_better(env, obj)) {
      objective_update(eval(env, obj));
      print_solution(stdout, env);
      solutions++;
    }
  }
}

