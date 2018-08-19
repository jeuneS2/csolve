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

static enum objective_t _objective;
static struct val_t _objective_val;
static volatile domain_t *_objective_best;

void objective_init(enum objective_t o, volatile domain_t *best) {
  _objective = o;
  _objective_val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  _objective_best = best;
  switch (o) {
  case OBJ_ANY:
  case OBJ_ALL:
    *_objective_best = 0;
    break;
  case OBJ_MIN:
    *_objective_best = DOMAIN_MAX;
    break;
  case OBJ_MAX:
    *_objective_best = DOMAIN_MIN;
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

enum objective_t objective(void) {
  return _objective;
}

bool objective_better(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    return true;
  case OBJ_MIN:
    return get_lo(_objective_val) < objective_best();
  case OBJ_MAX:
    return get_hi(_objective_val) > objective_best();
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
  return true;
}

void objective_update_best(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    break;
  case OBJ_MIN:
    *_objective_best = get_lo(_objective_val);
    break;
  case OBJ_MAX:
    *_objective_best = get_hi(_objective_val);
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

void objective_update_val(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    break;
  case OBJ_MIN: {
    domain_t best = objective_best();
    if (get_hi(_objective_val) > add(best, neg(1))) {
      _objective_val.hi = add(best, neg(1));
    }
    break;
  }
  case OBJ_MAX: {
    domain_t best = objective_best();
    if (get_lo(_objective_val) < add(best, 1)) {
      _objective_val.lo = add(best, 1);
    }
    break;
  }
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

struct val_t *objective_val(void) {
  return &_objective_val;
}

domain_t objective_best(void) {
  return *_objective_best;
}
