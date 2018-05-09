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

static bool _prefer_failing;
static bool _compute_weights;
static uint64_t _restart_frequency;
static enum order_t _order;


void strategy_prefer_failing_init(bool prefer_failing) {
  _prefer_failing = prefer_failing;
}

bool strategy_prefer_failing(void) {
  return _prefer_failing;
}

void strategy_compute_weights_init(bool compute_weights) {
  _compute_weights = compute_weights;
}

bool strategy_compute_weights(void) {
  return _compute_weights;
}

void strategy_restart_frequency_init(uint64_t restart_frequency) {
  _restart_frequency = restart_frequency;
}

uint64_t strategy_restart_frequency(void) {
  return _restart_frequency;
}

void strategy_order_init(enum order_t order) {
  _order = order;
}

domain_t strategy_pick_var_value(struct env_t *env, size_t depth) {
  switch (_order) {
  case ORDER_SMALLEST_DOMAIN:
    return add(get_hi(*env[depth].val), neg(get_lo(*env[depth].val)));
  case ORDER_LARGEST_DOMAIN:
    return add(get_lo(*env[depth].val), neg(get_hi(*env[depth].val)));
  case ORDER_SMALLEST_VALUE:
    return get_lo(*env[depth].val);
  case ORDER_LARGEST_VALUE:
    return neg(get_hi(*env[depth].val));
  default:
    print_error(ERROR_MSG_INVALID_STRATEGY_ORDER, _order);
    return 0;
  }
}

void strategy_pick_var(struct env_t *env, size_t depth) {
  if (_order != ORDER_NONE && env[depth].key != NULL) {
    domain_t value = strategy_pick_var_value(env, depth);
    size_t best_depth = depth;
    for (size_t i = depth; env[i].key != NULL; i++) {
      domain_t v = strategy_pick_var_value(env, i);
      if (v < value) {
        best_depth = i;
        value = v;
      }
    }
    struct env_t t = env[depth];
    env[depth] = env[best_depth];
    env[best_depth] = t;
  }
}
