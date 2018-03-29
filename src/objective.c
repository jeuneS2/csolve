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
static domain_t _objective_best;

void objective_init(enum objective_t o) {
  _objective = o;
  switch (o) {
  case OBJ_ANY:
  case OBJ_ALL:
    _objective_best = 0;
    break;
  case OBJ_MIN:
    _objective_best = DOMAIN_MAX;
    break;
  case OBJ_MAX:
    _objective_best = DOMAIN_MIN;
    break;
  default:
    fprintf(stderr, ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

enum objective_t objective(void) {
  return _objective;
}

bool objective_better(struct env_t *env, struct constr_t *obj) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    return true;
    break;
  case OBJ_MIN:
    if (_objective_best != DOMAIN_MAX) {
      struct val_t o = eval(env, obj);
      return get_lo(o) < _objective_best;
    }
    break;
  case OBJ_MAX:
    if (_objective_best != DOMAIN_MIN) {
      struct val_t o = eval(env, obj);
      return get_hi(o) > _objective_best;
    }
    break;
  default:
    fprintf(stderr, ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
  return true;
}

void objective_update(struct val_t obj) {
  if (is_value(obj)) {
    _objective_best = obj.value.val;
  } else {
    fprintf(stderr, ERROR_MSG_UPDATE_BEST_WITH_INTERVAL);
  }
}

domain_t objective_best(void) {
  return _objective_best;
}

struct constr_t *objective_optimize(struct env_t *env, struct constr_t *obj) {
  struct constr_t *retval = obj;
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    break;
  case OBJ_MIN:
    retval = normalize(env, retval);
    retval = propagate(env, retval, INTERVAL(DOMAIN_MIN, add(_objective_best, neg(1))));
    break;
  case OBJ_MAX:
    retval = normalize(env, retval);
    retval = propagate(env, retval, INTERVAL(add(_objective_best, 1), DOMAIN_MAX));
    break;
  default:
    fprintf(stderr, ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
  return retval;
}
