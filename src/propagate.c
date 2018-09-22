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

prop_result_t propagate_term(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct val_t term = constr->constr.term.val;
  struct env_t *var = constr->constr.term.env;

  if (get_lo(term) > get_hi(val) || get_hi(term) < get_lo(val)) {
    if (var != NULL) {
      var->prio++;
      strategy_var_order_update(var);
      if (strategy_create_conflicts()) {
        conflict_create(var, clause);
      }
    }
    return PROP_ERROR;
  } else {
    domain_t lo = max(get_lo(term), get_lo(val));
    domain_t hi = min(get_hi(term), get_hi(val));
    if (lo != get_lo(term) || hi != get_hi(term)) {
      struct val_t v = INTERVAL(lo, hi);
      if (var != NULL) {
        bind(var, v, clause);
        stat_inc_props();
        prop_result_t p = propagate_clauses(&var->clauses);
        if (p == PROP_ERROR) {
          var->prio++;
          strategy_var_order_update(var);
          return PROP_ERROR;
        }
        return p + 1;
      } else {
        constr->constr.term.val = v;
        return 1;
      }
    }
  }

  return PROP_NONE;
}

prop_result_t propagate_eq(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = l->type->eval(l);
    prop_result_t p = r->type->prop(r, lval, clause);
    CHECK(p);

    struct val_t rval = r->type->eval(r);
    prop_result_t q = l->type->prop(l, rval, clause);
    CHECK(q);

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_lt(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    struct val_t lval = l->type->eval(l);
    prop_result_t p = r->type->prop(r, INTERVAL(add(get_lo(lval), 1), DOMAIN_MAX), clause);
    CHECK(p);

    struct val_t rval = r->type->eval(r);
    prop_result_t q = l->type->prop(l, INTERVAL(DOMAIN_MIN, add(get_hi(rval), neg(1))), clause);
    CHECK(q);

    return p + q;

  } else if (is_false(val)) {

    struct val_t lval = l->type->eval(l);
    prop_result_t p = r->type->prop(r, INTERVAL(DOMAIN_MIN, get_hi(lval)), clause);
    CHECK(p);

    struct val_t rval = r->type->eval(r);
    prop_result_t q = l->type->prop(l, INTERVAL(get_lo(rval), DOMAIN_MAX), clause);
    CHECK(q);

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_neg(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;

  domain_t lo = neg(get_hi(val));
  domain_t hi = neg(get_lo(val));
  struct val_t v = INTERVAL(lo, hi);

  return l->type->prop(l, v, clause);
}

prop_result_t propagate_add(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  struct val_t lval = l->type->eval(l);
  domain_t rlo = add(get_lo(val), neg(get_hi(lval)));
  domain_t rhi = add(get_hi(val), neg(get_lo(lval)));
  struct val_t rv = INTERVAL(rlo, rhi);
  prop_result_t p = r->type->prop(r, rv, clause);
  CHECK(p);

  struct val_t rval = r->type->eval(r);
  domain_t llo = add(get_lo(val), neg(get_hi(rval)));
  domain_t lhi = add(get_hi(val), neg(get_lo(rval)));
  struct val_t lv = INTERVAL(llo, lhi);
  prop_result_t q = l->type->prop(l, lv, clause);
  CHECK(q);

  return p + q;
}

prop_result_t propagate_mul_lr(struct constr_t *p, struct constr_t *c, struct val_t val, const struct wand_expr_t *clause) {
  if (get_lo(val) != DOMAIN_MIN && get_hi(val) != DOMAIN_MIN) {
    struct val_t cval = c->type->eval(c);
    if (is_value(cval)) {
      if (((get_lo(val) > 0 || get_hi(val) < 0) && get_lo(cval) == 0) ||
          (is_value(val) && get_lo(cval) != 0 && (get_lo(val) % get_lo(cval)) != 0)) {
        return PROP_ERROR;
      } else if (get_lo(cval) != 0) {
        domain_t lo = get_lo(val) / get_lo(cval);
        domain_t hi = get_hi(val) / get_lo(cval);
        return p->type->prop(p, INTERVAL(min(lo, hi), max(lo, hi)), clause);
      }
    }
  }
  return PROP_NONE;
}

prop_result_t propagate_mul(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  prop_result_t p = propagate_mul_lr(r, l, val, clause);
  CHECK(p);

  prop_result_t q = propagate_mul_lr(l, r, val, clause);
  CHECK(q);

  return p + q;
}

prop_result_t propagate_not(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;

  if (is_true(val)) {
    return l->type->prop(l, VALUE(0), clause);
  } else if (is_false(val)) {
    return l->type->prop(l, VALUE(1), clause);
  }

  return PROP_NONE;
}

prop_result_t propagate_and(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_true(val)) {
    prop_result_t p = r->type->prop(r, val, clause);
    CHECK(p);

    prop_result_t q = l->type->prop(l, val, clause);
    CHECK(q);

    return p + q;

  } else if (is_false(val)) {
    prop_result_t p = PROP_NONE;
    struct val_t lval = l->type->eval(l);
    if (is_true(lval)) {
      p = r->type->prop(r, val, clause);
      CHECK(p);
    }

    prop_result_t q = PROP_NONE;
    struct val_t rval = r->type->eval(r);
    if (is_true(rval)) {
      q = l->type->prop(l, val, clause);
      CHECK(q);
    }

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_or(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  if (is_false(val)) {
    prop_result_t p = r->type->prop(r, val, clause);
    CHECK(p);

    prop_result_t q = l->type->prop(l, val, clause);
    CHECK(q);

    return p + q;

  } else if (is_true(val)) {
    prop_result_t p = PROP_NONE;
    struct val_t lval = l->type->eval(l);
    if (is_false(lval)) {
      p = r->type->prop(r, val, clause);
      CHECK(p);
    }

    prop_result_t q = PROP_NONE;
    struct val_t rval = r->type->eval(r);
    if (is_false(rval)) {
      q = l->type->prop(l, val, clause);
      CHECK(q);
    }

    return p + q;
  }

  return PROP_NONE;
}

prop_result_t propagate_wand(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  prop_result_t r = PROP_NONE;
  if (is_true(val)) {
    for (size_t i = 0; i < constr->constr.wand.length; i++) {
      struct constr_t *c = constr->constr.wand.elems[i].constr;
      prop_result_t p = c->type->prop(c, val, clause);
      CHECK(p);
      r += p;
    }
  }
  return r;
}

prop_result_t propagate_confl(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct confl_elem_t *p = NULL;
  
  for (size_t i = 0; i < constr->constr.confl.length; i++) {

    struct confl_elem_t *c = &constr->constr.confl.elems[i];
    const struct val_t v = c->var->type->eval(c->var);

    if (is_value(v)) {
      if (get_lo(v) != get_lo(c->val)) {
        return PROP_NONE;
      }
    } else {
      if (p == NULL) {
        p = c;
      } else {
        return PROP_NONE;
      }
    }
  }

  if (p != NULL) {
    const struct val_t v = p->var->type->eval(p->var);
    if (get_lo(v) == get_lo(p->val)) {
      return p->var->type->prop(p->var, INTERVAL(add(get_lo(v), 1), get_hi(v)), clause);
    }
    if (get_hi(v) == get_hi(p->val)) {
      return p->var->type->prop(p->var, INTERVAL(get_lo(v), add(get_hi(v), neg(1))), clause);
    }
  }
  
  return PROP_NONE;
}

prop_result_t propagate(struct constr_t *constr) {
  prop_result_t r = PROP_NONE;
  prop_result_t p = PROP_NONE;
  do {
    p = constr->type->prop(constr, VALUE(1), NULL);
    CHECK(p);
    r += p;
  } while (p != PROP_NONE);
  return r;
}

prop_result_t propagate_clauses(const struct clause_list_t *clauses) {
  static prop_tag_t _prop_tag = 0;
  prop_tag_t tag = ++_prop_tag;

  prop_result_t r = PROP_NONE;

  if (clauses->length > 0) {
    conflict_reset();
  }
  
  for (size_t i = 0; i < clauses->length; i++) {
    struct wand_expr_t *clause = clauses->elems[i];
    if (clause->prop_tag > tag) {
      continue;
    }
    clause->prop_tag = tag;

    struct constr_t *c = clause->constr;
    prop_result_t p = c->type->prop(c, VALUE(1), clause);
    CHECK(p);
    r += p;

    if (p != PROP_NONE) {
      struct constr_t *norm = c->type->norm(c);
      if (norm != c) {
        patch(clause, norm);
      }
    }
  }

  return r;
}
