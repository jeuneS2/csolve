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

#define CHECK(VAR)                              \
  if (VAR == PROP_ERROR) {                      \
    return PROP_ERROR;                          \
  }

prop_result_t prop(struct constr_t *constr, struct val_t val);

prop_result_t propagate_term(struct constr_t *constr, struct val_t val) {
  struct val_t *term = constr->constr.term;

  if (get_lo(*term) > get_hi(val) || get_hi(*term) < get_lo(val)) {
    if (term->env != NULL) {
      term->env->prio++;
      strategy_var_order_update(term->env);
    }
    return PROP_ERROR;
  } else {
    domain_t lo = max(get_lo(*term), get_lo(val));
    domain_t hi = min(get_hi(*term), get_hi(val));
    if (lo != get_lo(*term) || hi != get_hi(*term)) {
      struct val_t v = INTERVAL(lo, hi);
      bind(term, v);
      stat_inc_props();
      if (term->env != NULL) {
        prop_result_t p = propagate_clauses(term->env->clauses);
        if (p == PROP_ERROR) {
          term->env->prio++;
          strategy_var_order_update(term->env);
          return PROP_ERROR;
        }
        return p + 1;
      } else {
        return 1;
      }
    }
  }

  return PROP_NONE;
}

prop_result_t propagate_eq(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = eval(l);
    prop_result_t p = prop(r, lval);
    CHECK(p);

    struct val_t rval = eval(r);
    prop_result_t q = prop(l, rval);
    CHECK(q);

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_lt(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = eval(l);
    prop_result_t p = prop(r, INTERVAL(add(get_lo(lval), 1), DOMAIN_MAX));
    CHECK(p);

    struct val_t rval = eval(r);
    prop_result_t q = prop(l, INTERVAL(DOMAIN_MIN, add(get_hi(rval), neg(1))));
    CHECK(q);

    return p + q;

  } else if (is_false(val)) {

    struct val_t lval = eval(l);
    prop_result_t p = prop(r, INTERVAL(DOMAIN_MIN, get_hi(lval)));
    CHECK(p);

    struct val_t rval = eval(r);
    prop_result_t q = prop(l, INTERVAL(get_lo(rval), DOMAIN_MAX));
    CHECK(q);

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_neg(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;

  domain_t lo = neg(get_hi(val));
  domain_t hi = neg(get_lo(val));
  struct val_t v = INTERVAL(lo, hi);

  return  prop(l, v);
}

prop_result_t propagate_add(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  struct val_t lval = eval(l);
  domain_t rlo = add(get_lo(val), neg(get_hi(lval)));
  domain_t rhi = add(get_hi(val), neg(get_lo(lval)));
  struct val_t rv = INTERVAL(rlo, rhi);
  prop_result_t p = prop(r, rv);
  CHECK(p);

  struct val_t rval = eval(r);
  domain_t llo = add(get_lo(val), neg(get_hi(rval)));
  domain_t lhi = add(get_hi(val), neg(get_lo(rval)));
  struct val_t lv = INTERVAL(llo, lhi);
  prop_result_t q = prop(l, lv);
  CHECK(q);

  return p + q;
}

prop_result_t propagate_mul_lr(struct constr_t *p, struct constr_t *c, struct val_t val) {
  if (get_lo(val) != DOMAIN_MIN && get_hi(val) != DOMAIN_MIN) {
    struct val_t cval = eval(c);
    if (is_value(cval)) {
      if (((get_lo(val) > 0 || get_hi(val) < 0) && get_lo(cval) == 0) ||
          (is_value(val) && get_lo(cval) != 0 && (get_lo(val) % get_lo(cval)) != 0)) {
        return PROP_ERROR;
      } else if (get_lo(cval) != 0) {
        domain_t lo = get_lo(val) / get_lo(cval);
        domain_t hi = get_hi(val) / get_lo(cval);
        return prop(p, INTERVAL(min(lo, hi), max(lo, hi)));
      }
    }
  }
  return PROP_NONE;
}

prop_result_t propagate_mul(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  prop_result_t p = propagate_mul_lr(r, l, val);
  CHECK(p);

  prop_result_t q = propagate_mul_lr(l, r, val);
  CHECK(q);

  return p + q;
}

prop_result_t propagate_not(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;

  if (is_true(val)) {
    return prop(l, VALUE(0));
  } else if (is_false(val)) {
    return prop(l, VALUE(1));
  }

  return PROP_NONE;
}

prop_result_t propagate_and(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    prop_result_t p = prop(r, val);
    CHECK(p);

    prop_result_t q = prop(l, val);
    CHECK(q);

    return p + q;

  } else if (is_false(val)) {
    prop_result_t p = PROP_NONE;
    struct val_t lval = eval(l);
    if (is_true(lval)) {
      p = prop(r, val);
      CHECK(p);
    }

    prop_result_t q = PROP_NONE;
    struct val_t rval = eval(r);
    if (is_true(rval)) {
      q = prop(l, val);
      CHECK(q);
    }

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_or(struct constr_t *constr, struct val_t val) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_false(val)) {
    prop_result_t p = prop(r, val);
    CHECK(p);

    prop_result_t q = prop(l, val);
    CHECK(q);

    return p + q;

  } else if (is_true(val)) {
    prop_result_t p = PROP_NONE;
    struct val_t lval = eval(l);
    if (is_false(lval)) {
      p = prop(r, val);
      CHECK(p);
    }

    prop_result_t q = PROP_NONE;
    struct val_t rval = eval(r);
    if (is_false(rval)) {
      q = prop(l, val);
      CHECK(q);
    }

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_wand(struct constr_t *constr, struct val_t val) {
  prop_result_t r = PROP_NONE;
  if (is_true(val)) {
    for (size_t i = 0; i < constr->constr.wand.length; i++) {
      prop_result_t p = prop(constr->constr.wand.elems[i].constr, val);
      CHECK(p);
      r += p;
    }
  }
  return r;
}

prop_result_t prop(struct constr_t *constr, struct val_t val) {
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
  return PROP_NONE;
}

prop_result_t propagate(struct constr_t *constr, struct val_t val) {
  prop_result_t r = PROP_NONE;
  prop_result_t p = PROP_NONE;
  do {
    p = prop(constr, val);
    CHECK(p);
    r += p;
  } while (p != PROP_NONE);
  return r;
}

prop_result_t propagate_clauses(struct clause_list_t *clauses) {
  static prop_tag_t _prop_tag = 0;

  prop_result_t r = PROP_NONE;

  prop_tag_t tag = ++_prop_tag;
  for (struct clause_list_t *l = clauses; l != NULL; l = l->next) {
    l->clause->prop_tag = tag;
  }

  for (struct clause_list_t *l = clauses; l != NULL; l = l->next) {
    struct wand_expr_t *clause = l->clause;
    if (clause->prop_tag != tag) {
      continue;
    }

    prop_result_t p = prop(clause->constr, VALUE(1));
    CHECK(p);
    r += p;

    if (p != PROP_NONE) {
      struct constr_t *norm = normalize_step(clause->constr);
      if (norm != clause->constr) {
        patch(clause, (struct wand_expr_t){ norm });
      }
    }
  }

  return r;
}
