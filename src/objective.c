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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "csolve.h"

// what solution to look for
static enum objective_t _objective;
// the objective value constraint
static struct constr_t _objective_val;
// the value of the best solution found so far
static volatile domain_t *_objective_best;

// initialize the solution to look for
void objective_init(enum objective_t o, volatile domain_t *best) {
  _objective = o;
  _objective_val = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN+1, DOMAIN_MAX-1));
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

// return what solution to look for
enum objective_t objective(void) {
  return _objective;
}

// check whether value of objective value constraint is possibly
// better than the best objective value found so far
bool objective_better(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    // objective value is alway "better" when looking for all/any solutions
    return true;
  case OBJ_MIN:
    // compare lower bound when looking for minimum
    return get_lo(_objective_val.constr.term.val) < objective_best();
  case OBJ_MAX:
    // compare upper bound when looking for maximum
    return get_hi(_objective_val.constr.term.val) > objective_best();
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
  return true;
}

// update the best objective value found so far
void objective_update_best(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    // no objective value when looking for all/any solutions
    break;
  case OBJ_MIN:
    // update objective value with lower bound when looking for minimum
    *_objective_best = get_lo(_objective_val.constr.term.val);
    break;
  case OBJ_MAX:
    // update objective value with upper bound when looking for maximum
    *_objective_best = get_hi(_objective_val.constr.term.val);
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

// update the objective value constraint
void objective_update_val(void) {
  switch (_objective) {
  case OBJ_ANY:
  case OBJ_ALL:
    // nothing to update when looking for all/any solutions
    break;
  case OBJ_MIN: {
    // constrain upper bound when looking for minimum
    domain_t best = objective_best();
    if (get_hi(_objective_val.constr.term.val) > add(best, neg(1))) {
      _objective_val.constr.term.val.hi = add(best, neg(1));
    }
    break;
  }
  case OBJ_MAX: {
    // constrain lower bound when looking for maximum
    domain_t best = objective_best();
    if (get_lo(_objective_val.constr.term.val) < add(best, 1)) {
      _objective_val.constr.term.val.lo = add(best, 1);
    }
    break;
  }
  default:
    print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, _objective);
  }
}

// return a pointer to the objective value constraint
struct constr_t *objective_val(void) {
  return &_objective_val;
}

// return the best objective value found so far
domain_t objective_best(void) {
  return *_objective_best;
}
