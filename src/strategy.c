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

int strategy_pick_var_cmp(struct env_t *env, size_t depth1, size_t depth2) {
  struct val_t v1 = *env[depth1].val;
  struct val_t v2 = *env[depth2].val;

  int cmp = 0;
  switch (_order) {
  case ORDER_SMALLEST_DOMAIN: {
    domain_t d1 = add(get_lo(v1), neg(get_hi(v1)));
    domain_t d2 = add(get_hi(v2), neg(get_lo(v2)));
    cmp = add(d2, d1);
    break;
  }
  case ORDER_LARGEST_DOMAIN: {
    domain_t d1 = add(get_hi(v1), neg(get_lo(v1)));
    domain_t d2 = add(get_lo(v2), neg(get_hi(v2)));
    cmp = add(d1, d2);
    break;
  }
  case ORDER_SMALLEST_VALUE:
    cmp = add(get_lo(v2), neg(get_lo(v1)));
    break;
  case ORDER_LARGEST_VALUE:
    cmp = add(get_hi(v1), neg(get_hi(v2)));
    break;
  case ORDER_NONE:
    cmp = 0;
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_STRATEGY_ORDER, _order);
  }

  if (strategy_prefer_failing() && cmp == 0) {
    cmp = env[depth1].fails - env[depth2].fails;
  }

  return cmp;
}

void strategy_pick_var(struct env_t *env, size_t depth) {
  if (env[depth].key != NULL) {
    size_t best_depth = depth;
    for (size_t i = depth; env[i].key != NULL; i++) {
      if (strategy_pick_var_cmp(env, i, best_depth) > 0) {
        best_depth = i;
      }
    }
    swap_env(env, depth, best_depth);
  }
}
