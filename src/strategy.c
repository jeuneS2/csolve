/* Copyright 2018-2019 Wolfgang Puffitsch

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

#include "csolve.h"

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool _create_conflicts;
static bool _prefer_failing;
static bool _compute_weights;
static uint64_t _restart_frequency;
static enum order_t _order;

// initialize whether to create conflicts
void strategy_create_conflicts_init(bool create_conflicts) {
  _create_conflicts = create_conflicts;
}

// return whether to create conflicts
bool strategy_create_conflicts(void) {
  return _create_conflicts;
}

// initialize whether to prefer variables where assignments failed
void strategy_prefer_failing_init(bool prefer_failing) {
  _prefer_failing = prefer_failing;
}

// return whether to prefer variables where assignments failed
bool strategy_prefer_failing(void) {
  return _prefer_failing;
}

// initialize whether to compute initial weights
void strategy_compute_weights_init(bool compute_weights) {
  _compute_weights = compute_weights;
}

// return whether to compute initial weights
bool strategy_compute_weights(void) {
  return _compute_weights;
}

// initialize restart frequency
void strategy_restart_frequency_init(uint64_t restart_frequency) {
  _restart_frequency = restart_frequency;
}

// return retstart frequency
uint64_t strategy_restart_frequency(void) {
  return _restart_frequency;
}

// initialize the variable ordering
void strategy_order_init(enum order_t order) {
  _order = order;
}

// compare two variables according to the variable ordering
static int strategy_var_cmp(struct env_t *e1, struct env_t *e2) {
  struct val_t v1 = e1->val->constr.term.val;
  struct val_t v2 = e2->val->constr.term.val;

  int cmp = 0;
  switch (_order) {
  case ORDER_SMALLEST_DOMAIN: {
    // compare domain sizes and prefer the smaller one
    domain_t d1 = add(get_lo(v1), neg(get_hi(v1)));
    domain_t d2 = add(get_hi(v2), neg(get_lo(v2)));
    cmp = add(d2, d1);
    break;
  }
  case ORDER_LARGEST_DOMAIN: {
    // compare domain sizes and prefer the larger one
    domain_t d1 = add(get_hi(v1), neg(get_lo(v1)));
    domain_t d2 = add(get_lo(v2), neg(get_hi(v2)));
    cmp = add(d1, d2);
    break;
  }
  case ORDER_SMALLEST_VALUE:
    // compare the lower bounds and prefer the smaller one
    cmp = add(get_lo(v2), neg(get_lo(v1)));
    break;
  case ORDER_LARGEST_VALUE:
    // compare the upper bounds and prefer the larger one
    cmp = add(get_hi(v1), neg(get_hi(v2)));
    break;
  case ORDER_NONE:
    // do not use domains or values for comparison
    cmp = 0;
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_STRATEGY_ORDER, _order);
  }

  // break ties through variable priority if enabled
  if (strategy_prefer_failing() && cmp == 0) {
    cmp = e1->prio - e2->prio;
  }

  return cmp;
}

// definitions for priority queue of variables
#define VAR_ORDER_HEAP_ARITY 2
size_t _var_order_size;
struct env_t **_var_order;

// get index of parent of priority queue entry
static inline size_t parent(size_t child) {
  return (child - 1) / VAR_ORDER_HEAP_ARITY;
}

// get index of left child of priority queue entry
static inline size_t left(size_t parent) {
  return VAR_ORDER_HEAP_ARITY * parent + 1;
}

// get index of right child of priority queue entry
static inline size_t right(size_t parent) {
  return VAR_ORDER_HEAP_ARITY * parent + 2;
}

// print the priority queue of variables
void strategy_var_order_print(FILE *file, size_t pos) {
  if (pos < _var_order_size) {
    fprintf(file, "(%s %lu ", _var_order[pos]->key, _var_order[pos]->prio);
    strategy_var_order_print(file, left(pos));
    strategy_var_order_print(file, right(pos));
    fprintf(file, ")");
  }
}

// initialize the priority queue of variables
void strategy_var_order_init(size_t size, struct env_t *env) {
  _var_order_size = 0;
  _var_order = (struct env_t **)malloc(size * sizeof(struct env_t *));

  // insert all variables from environment into priority queue of variables
  for (size_t i = 0; i < size; i++) {
    strategy_var_order_push(&env[i]);
  }
}

// release memory for priority queue of variables
void strategy_var_order_free(void) {
  free(_var_order);
}

// swap two variables in priority queue of variables
static void strategy_var_order_swap(size_t pos1, size_t pos2) {
  struct env_t *t = _var_order[pos1];
  _var_order[pos1] = _var_order[pos2];
  _var_order[pos1]->order = pos1;
  _var_order[pos2] = t;
  _var_order[pos2]->order = pos2;
}

// move up entry in priority queue of variables
static void strategy_var_order_up(size_t pos) {
  // swap positions if parent position is better
  while (pos > 0 && strategy_var_cmp(_var_order[parent(pos)], _var_order[pos]) < 0) {
    strategy_var_order_swap(pos, parent(pos));
    pos = parent(pos);
  }
}

// move down entry in priority queue of variables
static void strategy_var_order_down(size_t pos) {
  while (true) {
    size_t lpos = left(pos);
    size_t rpos = right(pos);

    size_t best = pos;
    // check if position of left child is better
    if (lpos < _var_order_size && strategy_var_cmp(_var_order[lpos], _var_order[best]) > 0) {
      best = lpos;
    }
    // check if position of right child is better
    if (rpos < _var_order_size && strategy_var_cmp(_var_order[rpos], _var_order[best]) > 0) {
      best = rpos;
    }
    if (best != pos) {
      // swap if the position is to be updated
      strategy_var_order_swap(best, pos);
      pos = best;
    } else {
      // otherwise, stop
      break;
    }
  }
}

// push variable into priority queue of variables
void strategy_var_order_push(struct env_t *e) {
  // add to end of queue
  size_t pos = _var_order_size++;
  _var_order[pos] = e;
  _var_order[pos]->order = pos;
  // move up to correct position
  strategy_var_order_up(pos);
}

// pop variable from priority queue of variables
struct env_t *strategy_var_order_pop(void) {
  // get head of priority queue
  struct env_t *retval = _var_order[0];
  retval->order = SIZE_MAX;
  --_var_order_size;

  // adjust ordering as needed by moving new head down as needed
  if (_var_order_size > 0) {
    _var_order[0] = _var_order[_var_order_size];
    _var_order[0]->order = 0;
    strategy_var_order_down(0);
  }
  return retval;
}

// update the position of a variable in the priority queue
void strategy_var_order_update(struct env_t *e) {
  if (e->order != SIZE_MAX) {
    // move up and down to find correct position
    strategy_var_order_up(e->order);
    strategy_var_order_down(e->order);
  }
}
