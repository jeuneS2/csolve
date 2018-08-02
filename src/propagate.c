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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "csolve.h"

static int32_t _prop_count = 0;

struct constr_t *prop(struct constr_t *constr, struct val_t val);

static inline struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l == NULL || r == NULL) {
    return NULL;
  }
  return constr;
}

static inline struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  if (l == NULL) {
    return NULL;
  }
  return constr;
}

struct constr_t *propagate_term(struct constr_t *constr, struct val_t val) {
  struct val_t *term = constr->constr.term;

  if (get_lo(*term) > get_hi(val) || get_hi(*term) < get_lo(val)) {
    return NULL;
  } else {
    domain_t lo = max(get_lo(*term), get_lo(val));
    domain_t hi = min(get_hi(*term), get_hi(val));
    if (lo != get_lo(*term) || hi != get_hi(*term)) {
      struct val_t v = (lo == hi) ? VALUE(lo) : INTERVAL(lo, hi);
      bind(term, v);
      _prop_count++;
    }
  }

  return constr;
}

struct constr_t *propagate_eq(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = eval(l);
    r = prop(r, lval);
    if (r != NULL) {
      struct val_t rval = eval(r);
      l = prop(l, rval);
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_lt(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = eval(l);
    r = prop(r, INTERVAL(add(get_lo(lval), 1), DOMAIN_MAX));
    if (r != NULL) {
      struct val_t rval = eval(r);
      l = prop(l, INTERVAL(DOMAIN_MIN, add(get_hi(rval), neg(1))));
    }
  } else if (is_false(val)) {
    struct val_t lval = eval(l);
    r = prop(r, INTERVAL(DOMAIN_MIN, get_hi(lval)));
    if (r != NULL) {
      struct val_t rval = eval(r);
      l = prop(l, INTERVAL(get_lo(rval), DOMAIN_MAX));
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_neg(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;

  domain_t lo = neg(get_hi(val));
  domain_t hi = neg(get_lo(val));
  struct val_t v = (lo == hi) ? VALUE(lo) : INTERVAL(lo, hi);
  l = prop(l, v);

  return update_unary_expr(constr, l);
}

struct constr_t *propagate_add(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  struct val_t lval = eval(l);
  domain_t rlo = add(get_lo(val), neg(get_hi(lval)));
  domain_t rhi = add(get_hi(val), neg(get_lo(lval)));
  struct val_t rv = (rlo == rhi) ? VALUE(rlo) : INTERVAL(rlo, rhi);
  r = prop(r, rv);

  if (r != NULL) {
    struct val_t rval = eval(r);
    domain_t llo = add(get_lo(val), neg(get_hi(rval)));
    domain_t lhi = add(get_hi(val), neg(get_lo(rval)));
    struct val_t lv = (llo == lhi) ? VALUE(llo) : INTERVAL(llo, lhi);
    l = prop(l, lv);
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_mul_lr(struct constr_t *p, struct constr_t *c, struct val_t val) {
  if (get_lo(val) != DOMAIN_MIN && get_hi(val) != DOMAIN_MIN) {
    struct val_t cval = eval(c);
    if (is_value(cval)) {
      if (((get_lo(val) > 0 || get_hi(val) < 0) && get_lo(cval) == 0) ||
          (is_value(val) && get_lo(cval) != 0 && (get_lo(val) % get_lo(cval)) != 0)) {
        return NULL;
      } else if (get_lo(cval) != 0) {
        domain_t lo = get_lo(val) / get_lo(cval);
        domain_t hi = get_hi(val) / get_lo(cval);
        return prop(p, INTERVAL(min(lo, hi), max(lo, hi)));
      }
    }
  }
  return p;
}

struct constr_t *propagate_mul(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  r = propagate_mul_lr(r, l, val);
  if (r != NULL) {
    l = propagate_mul_lr(l, r, val);
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_not(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;

  if (is_true(val)) {
    l = prop(l, VALUE(0));
  } else if (is_false(val)) {
    l = prop(l, VALUE(1));
  }

  return update_unary_expr(constr, l);
}

struct constr_t *propagate_and(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    r = prop(r, val);
    if (r != NULL) {
      l = prop(l, val);
    }
  } else if (is_false(val)) {
    struct val_t lval = eval(l);
    if (is_true(lval)) {
      r = prop(r, val);
    }
    if (r != NULL) {
      struct val_t rval = eval(r);
      if (is_true(rval)) {
        l = prop(l, val);
      }
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_or(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_false(val)) {
    r = prop(r, val);
    if (r != NULL) {
      l = prop(l, val);
    }
  } else if (is_true(val)) {
    struct val_t lval = eval(l);
    if (is_false(lval)) {
      r = prop(r, val);
    }
    if (r != NULL) {
      struct val_t rval = eval(r);
      if (is_false(rval)) {
        l = prop(l, val);
      }
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *propagate_wand(struct constr_t *constr, struct val_t val) {
  if (is_true(val)) {
    for (size_t i = 0; i < constr->constr.wand.length; i++) {
      struct constr_t *p = prop(constr->constr.wand.elems[i], val);
      if (p == NULL) {
        return NULL;
      }
    }
  }
  return constr;
}

struct constr_t *prop(struct constr_t *constr, struct val_t val) {
  switch (constr->type) {
  case CONSTR_TERM:
    return propagate_term(constr, val);
  case CONSTR_EXPR:
    switch (constr->constr.expr.op) {
    case OP_EQ:  return propagate_eq(constr, val);
    case OP_LT:  return propagate_lt(constr, val);
    case OP_NEG: return propagate_neg(constr, val);
    case OP_ADD: return propagate_add(constr, val);
    case OP_MUL: return propagate_mul(constr, val);
    case OP_NOT: return propagate_not(constr, val);
    case OP_AND: return propagate_and(constr, val);
    case OP_OR:  return propagate_or(constr, val);
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  case CONSTR_WAND:
    return propagate_wand(constr, val);
  default:
    print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
  }
  return constr;
}

struct constr_t *propagate(struct constr_t *constr, struct val_t val) {
  struct constr_t *retval = constr;
  do {
    _prop_count = 0;
    retval = prop(retval, val);
  } while (retval != NULL && _prop_count > 0);
  return retval;
}
