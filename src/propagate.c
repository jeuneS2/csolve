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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// return if propagation resulted in an error
#define CHECK(VAR)                              \
  if ((VAR) == PROP_ERROR) {                    \
    return PROP_ERROR;                          \
  }

// update variable proprity and create a conflict
static void propagate_term_confl(struct env_t *var, const struct wand_expr_t *clause) {
  // update priority and variable ordering
  var->prio++;
  strategy_var_order_update(var);
  // create conflict, if enabled
  if (strategy_create_conflicts()) {
    conflict_create(var, clause);
  }
}

// propagate new variable value to all affected clauses
static prop_result_t propagate_term_recurse(struct env_t *var) {
  // propagate to all affected clauses
  prop_result_t p = propagate_clauses(&var->clauses);
  if (p == PROP_ERROR) {
    // update priorty and variable ordering
    var->prio++;
    strategy_var_order_update(var);
    return PROP_ERROR;
  }
  return p + 1;
}

// propagate value to terminal
prop_result_t propagate_term(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct val_t term = constr->constr.term.val;
  struct env_t *var = constr->constr.term.env;

  if (get_lo(term) > get_hi(val) || get_hi(term) < get_lo(val)) {
    // conflicting propagation
    if (var != NULL) {
      propagate_term_confl(var, clause);
    }
    return PROP_ERROR;
  }

  // possible propagation
  domain_t lo = max(get_lo(term), get_lo(val));
  domain_t hi = min(get_hi(term), get_hi(val));
  // propagate only if actually restricting value
  if (lo != get_lo(term) || hi != get_hi(term)) {
    struct val_t v = INTERVAL(lo, hi);
    if (var != NULL) {
      // recurse if variable is defined
      bind(var, v, clause);
      stat_inc_props();
      return propagate_term_recurse(var);
    }
    // just assign value if there is no variable
    constr->constr.term.val = v;
    return 1;
  }

  return PROP_NONE;
}

// propagate the value "true" to an equality expression
static prop_result_t propagate_eq_true(struct constr_t *l, struct constr_t *r, const struct wand_expr_t *clause) {

  // propagate left value to right side
  struct val_t lval = l->type->eval(l);
  prop_result_t p = r->type->prop(r, lval, clause);
  CHECK(p);

  // propagate right value to left side
  struct val_t rval = r->type->eval(r);
  prop_result_t q = l->type->prop(l, rval, clause);
  CHECK(q);

  return p + q;
}

// propagate the value "false" to left or right side of equality expression
static prop_result_t propagate_eq_false_lr(struct constr_t *p, struct val_t pval, struct val_t val, const struct wand_expr_t *clause) {

  // restrict if value of other side hits an interval boundary
  if (is_value(val) && get_lo(val) != DOMAIN_MIN && get_lo(val) != DOMAIN_MAX) {
    if (get_lo(val) == get_lo(pval)) {
      // restrict lower bound
      return p->type->prop(p, INTERVAL(get_lo(val) + 1, DOMAIN_MAX), clause);
    }
    if (get_lo(val) == get_hi(pval)) {
      // restrict upper bound
      return p->type->prop(p, INTERVAL(DOMAIN_MIN, get_lo(val) - 1), clause);
    }
  }
  return PROP_NONE;
}

// propagate the value "false" to an equality expression
static prop_result_t propagate_eq_false(struct constr_t *l, struct constr_t *r, const struct wand_expr_t *clause) {
  struct val_t lval = l->type->eval(l);
  struct val_t rval = r->type->eval(r);

  // propagate left value to right side
  prop_result_t p = propagate_eq_false_lr(r, rval, lval, clause);
  CHECK(p);

  // propagate right value to left side
  prop_result_t q = propagate_eq_false_lr(l, lval, rval, clause);
  CHECK(q);

  return p + q;
}

// propagate value to equality expression
prop_result_t propagate_eq(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // split propagation depending on whether propagating "true" or "false"
  if (is_true(val)) {
    return propagate_eq_true(l, r, clause);
  }
  if (is_false(val)) {
    return propagate_eq_false(l, r, clause);
  }

  return PROP_NONE;
}

// propagate the value "true" to a less-than expression
static prop_result_t propagate_lt_true(struct constr_t *l, struct constr_t *r, const struct wand_expr_t *clause) {

  // propagate left value to right side
  struct val_t lval = l->type->eval(l);
  prop_result_t p = PROP_NONE;
  if (get_lo(lval) != DOMAIN_MIN && get_lo(lval) != DOMAIN_MAX) {
    // restrict lower bound of right side
    p = r->type->prop(r, INTERVAL(get_lo(lval) + 1, DOMAIN_MAX), clause);
    CHECK(p);
  }

  // propagate right value to left side
  struct val_t rval = r->type->eval(r);
  prop_result_t q = PROP_NONE;
  if (get_hi(rval) != DOMAIN_MIN && get_hi(rval) != DOMAIN_MAX) {
    // restrict upper bound of left side
    q = l->type->prop(l, INTERVAL(DOMAIN_MIN, get_hi(rval) - 1), clause);
    CHECK(q);
  }

  return p + q;
}

// propagate the value "false" to a less-than expression
static prop_result_t propagate_lt_false(struct constr_t *l, struct constr_t *r, const struct wand_expr_t *clause) {

  // propagate left value to right side
  struct val_t lval = l->type->eval(l);
  prop_result_t p = r->type->prop(r, INTERVAL(DOMAIN_MIN, get_hi(lval)), clause);
  CHECK(p);

  // propagate right value to left side
  struct val_t rval = r->type->eval(r);
  prop_result_t q = l->type->prop(l, INTERVAL(get_lo(rval), DOMAIN_MAX), clause);
  CHECK(q);

  return p + q;
}

// propagate value to less-than expression
prop_result_t propagate_lt(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // split propagation depending on whether propagating "true" or "false"
  if (is_true(val)) {
    return propagate_lt_true(l, r, clause);
  }
  if (is_false(val)) {
    return propagate_lt_false(l, r, clause);
  }

  return PROP_NONE;
}

// propagate value to negation expression
prop_result_t propagate_neg(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;

  // flip variable bounds for propagation
  domain_t lo = neg(get_hi(val));
  domain_t hi = neg(get_lo(val));
  struct val_t v = INTERVAL(lo, hi);

  return l->type->prop(l, v, clause);
}

// propagate to left side or right side of addition expression
static prop_result_t propagate_add_lr(struct constr_t *p, struct constr_t *c, struct val_t val, const struct wand_expr_t *clause) {
  struct val_t cval = c->type->eval(c);

  // propagate value by subtracting value of "other" side from value to be propagated
  domain_t lo = add(get_lo(val), neg(get_hi(cval)));
  domain_t hi = add(get_hi(val), neg(get_lo(cval)));
  return p->type->prop(p, INTERVAL(lo, hi), clause);
}

// propagate value to addition expression
prop_result_t propagate_add(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // propagate left value to right side
  prop_result_t p = propagate_add_lr(r, l, val, clause);
  CHECK(p);

  // propagate right value to left side
  prop_result_t q = propagate_add_lr(l, r, val, clause);
  CHECK(q);

  return p + q;
}

// propagate to left side or right side of multiplication expression
static prop_result_t propagate_mul_lr(struct constr_t *p, struct constr_t *c, struct val_t val, const struct wand_expr_t *clause) {

  // only propagate if value is not saturated
  if (get_lo(val) != DOMAIN_MIN && get_hi(val) != DOMAIN_MIN) {
    struct val_t cval = c->type->eval(c);
    // only propagate if the "other" side has a value
    if (is_value(cval)) {
      if (((get_lo(val) > 0 || get_hi(val) < 0) && get_lo(cval) == 0) ||
          (is_value(val) && get_lo(cval) != 0 && (get_lo(val) % get_lo(cval)) != 0)) {
        // return an error if the propagation is not possible
        return PROP_ERROR;
      }
      if (get_lo(cval) != 0) {
        // propagate value
        domain_t lo = get_lo(val) / get_lo(cval);
        domain_t hi = get_hi(val) / get_lo(cval);
        return p->type->prop(p, INTERVAL(min(lo, hi), max(lo, hi)), clause);
      }
    }
  }
  return PROP_NONE;
}

// propagate value to multiplication expression
prop_result_t propagate_mul(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // propagate left value to right side
  prop_result_t p = propagate_mul_lr(r, l, val, clause);
  CHECK(p);

  // propagate right value to left side
  prop_result_t q = propagate_mul_lr(l, r, val, clause);
  CHECK(q);

  return p + q;
}

// propagate value to logical not expression
prop_result_t propagate_not(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;

  // flip true/false for propagation
  if (is_true(val)) {
    return l->type->prop(l, VALUE(0), clause);
  }
  if (is_false(val)) {
    return l->type->prop(l, VALUE(1), clause);
  }

  return PROP_NONE;
}

// propagate value to logic operation where both sub-expressions must
// return the same value
static prop_result_t propagate_logic_both(struct constr_t *l, struct constr_t *r, const struct val_t val, const struct wand_expr_t *clause) {

  // propagate value to the right side
  prop_result_t p = r->type->prop(r, val, clause);
  CHECK(p);

  // propagate value to the left side
  prop_result_t q = l->type->prop(l, val, clause);
  CHECK(q);

  return p + q;
}

// propagate value to logic operation where only one sub-expression is
// needed to return the value
static prop_result_t propagate_logic_either(struct constr_t *l, struct constr_t *r, const struct val_t val,
                                            bool (*is_neutral)(struct val_t), const struct wand_expr_t *clause) {

  // if left is the neutral element, propagate value to the right
  prop_result_t p = PROP_NONE;
  struct val_t lval = l->type->eval(l);
  if (is_neutral(lval)) {
    p = r->type->prop(r, val, clause);
    CHECK(p);
  }

  // if right is the neutral element, propagate value to the left
  prop_result_t q = PROP_NONE;
  struct val_t rval = r->type->eval(r);
  if (is_neutral(rval)) {
    q = l->type->prop(l, val, clause);
    CHECK(q);
  }

  return p + q;
}

// propagate value to logical and expression
prop_result_t propagate_and(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // split propagation depending on whether propagating "true" or "false"
  if (is_true(val)) {
    // both sides must be true for a logical and to be true
    return propagate_logic_both(l, r, val, clause);
  }
  if (is_false(val)) {
    // either side may be false for a logical and to be false
    return propagate_logic_either(l, r, val, is_true, clause);
  }

  return PROP_NONE;
}

// propagate value to logical or expression
prop_result_t propagate_or(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {
  struct constr_t *l = constr->constr.expr.l;
  struct constr_t *r = constr->constr.expr.r;

  // split propagation depending on whether propagating "true" or "false"
  if (is_false(val)) {
    // both sides must be false for a logical or to be false
    return propagate_logic_both(l, r, val, clause);
  }
  if (is_true(val)) {
    // either side may be true for a logical or to be true
    return propagate_logic_either(l, r, val, is_false, clause);
  }

  return PROP_NONE;
}

// propagate value to wide-and expression
prop_result_t propagate_wand(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {

  // only propagate "true" to wide-and sub-expressions
  prop_result_t r = PROP_NONE;
  if (is_true(val)) {
    for (size_t i = 0, l = constr->constr.wand.length; i < l; i++) {
      struct constr_t *c = constr->constr.wand.elems[i].constr;
      prop_result_t p = c->type->prop(c, val, clause);
      CHECK(p);
      r += p;
    }
  }
  return r;
}

// swap two conflict elements
static void propagate_confl_swap(struct confl_elem_t *a, struct confl_elem_t *b) {
  struct confl_elem_t t;
  t = *a;
  *a = *b;
  *b = t;
}

// find if single non-value variable exists in conflict
static struct confl_elem_t *propagate_confl_find(struct constr_t *constr) {
  struct confl_elem_t *p = NULL;

  // find whether there is a variable to be inferred
  for (size_t i = 0, l = constr->constr.confl.length; i < l; i++) {

    struct confl_elem_t *c = &constr->constr.confl.elems[i];
    const struct val_t v = c->var->constr.term.val;

    if (is_value(v)) {
      // stop if some variable is different from its conflict value
      if (get_lo(v) != get_lo(c->val)) {
        if (i > 0) {
          // swap stopping entry forward
          propagate_confl_swap(&constr->constr.confl.elems[0], c);
        }
        return NULL;
      }
    } else {
      if (p == NULL) {
        // remember if there is a non-value variable
        p = c;
      } else {
        // stop if there are more than one non-value variables
        if (i > 1) {
          // swap stopping entries forward
          propagate_confl_swap(&constr->constr.confl.elems[0], p);
          propagate_confl_swap(&constr->constr.confl.elems[1], c);
        }
        return NULL;
      }
    }
  }

  return p;
}

// infer value of variable in conflict
static prop_result_t propagate_confl_infer(struct confl_elem_t *p, const struct wand_expr_t *clause) {
  const struct val_t v = p->var->type->eval(p->var);

  // restrict variable value from lower bound
  if (get_lo(v) == get_lo(p->val) && get_lo(v) != DOMAIN_MIN && get_lo(v) != DOMAIN_MAX) {
    return p->var->type->prop(p->var, INTERVAL(get_lo(v) + 1, DOMAIN_MAX), clause);
  }

  // restrict variable value from upper bound
  if (get_hi(v) == get_hi(p->val) && get_hi(v) != DOMAIN_MIN && get_hi(v) != DOMAIN_MAX) {
    return p->var->type->prop(p->var, INTERVAL(DOMAIN_MIN, get_hi(v) - 1), clause);
  }

  return PROP_NONE;
}

// propagate value to conflict expression
prop_result_t propagate_confl(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause) {

  // only propagate "true" to conflict
  if (is_true(val)) {
    // find remaining non-value variable
    struct confl_elem_t *p = propagate_confl_find(constr);
    // infer value of remaining non-value variable
    if (p != NULL) {
      return propagate_confl_infer(p, clause);
    }
  }

  return PROP_NONE;
}

// propagate value "true" to expression
prop_result_t propagate(struct constr_t *constr) {
  prop_result_t r = PROP_NONE;
  prop_result_t p = PROP_NONE;
  // loop until there are no new propagations
  do {
    p = constr->type->prop(constr, VALUE(1), NULL);
    CHECK(p);
    r += p;
  } while (p != PROP_NONE);
  return r;
}

// propagate value "true" to expressions in clause list
prop_result_t propagate_clauses(const struct clause_list_t *clauses) {
  // update propagation tag
  static prop_tag_t _prop_tag = 0;
  prop_tag_t tag = ++_prop_tag;

  prop_result_t r = PROP_NONE;

  // reset conflicts
  conflict_reset();

  for (size_t i = 0, l = clauses->length; i < l; i++) {
    struct wand_expr_t *clause = clauses->elems[i];
    // skip if a later call already propagated this clause
    if (clause->prop_tag > tag) {
      continue;
    }
    // update propagation tag
    clause->prop_tag = tag;

    // propagate "true" to this clause
    struct constr_t *c = clause->constr;
    prop_result_t p = c->type->prop(c, VALUE(1), clause);
    CHECK(p);
    r += p;

    // if propagation happened, normalize and patch if applicable
    if (p != PROP_NONE) {
      struct constr_t *norm = c->type->norm(c);
      if (norm != c) {
        patch(clause, norm);
      }
    }
  }

  return r;
}
