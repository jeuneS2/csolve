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

static int32_t _patch_count = 0;

// check if constraint can be normalized through evaluation and return
// if this is the case
#define NORM_EVAL(CONSTR)                       \
  {                                             \
    struct constr_t *e = normal_eval(constr);   \
    if (e != constr) {                          \
      return e;                                 \
    }                                           \
  }

// return newly allocated expression if sub-expressions changed, or
// old expression if sub-expressions are unchanged
static struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l != constr->constr.expr.l || r != constr->constr.expr.r) {
    // allocate new expression, copy type, and fill in new sub-expressions
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.l = l;
    retval->constr.expr.r = r;
    return retval;
  }
  return constr;
}

// return newly allocated expression if sub-expression changed, or old
// expression if sub-expression is unchanged
static struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  if (l != constr->constr.expr.l) {
    // allocate new expression, copy type, and fill in new sub-expression
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.l = l;
    retval->constr.expr.r = NULL;
    return retval;
  }
  return constr;
}

// replace expression with constant if evaluation results in constant
static struct constr_t *normal_eval(struct constr_t *constr) {
  struct val_t val = constr->type->eval(constr);
  if (is_value(val)) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    *retval = CONSTRAINT_TERM(val);
    return retval;
  }
  return constr;
}

// normalize terminal
struct constr_t *normal_term(struct constr_t *constr) {
  return constr;
}

// normalize equality expression
struct constr_t *normal_eq(struct constr_t *constr) {
  NORM_EVAL(constr);

  // normalize sub-expressions
  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  // shortcut if both sides are the same
  if (l == r) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    *retval = CONSTRAINT_TERM(VALUE(1));
    return retval;
  }

  return update_expr(constr, l, r);
}

// normalize less-than expression
struct constr_t *normal_lt(struct constr_t *constr) {
  NORM_EVAL(constr);

  // normalize sub-expressions
  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  // shortcut if both sides are the same
  if (l == r) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    *retval = CONSTRAINT_TERM(VALUE(0));
    return retval;
  }

  // swap left and right if both sides are negations
  if (IS_TYPE(NEG, l) && IS_TYPE(NEG, r)) {
    return update_expr(constr, r->constr.expr.l, l->constr.expr.l);
  }

  if (is_const(l)) {

    // move constant of addition on right side to left side
    if (IS_TYPE(ADD, r) && is_const(r->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(NEG, r->constr.expr.r, NULL);
      c = r->type->norm(update_expr(r, l, c));

      return update_expr(constr, c, r->constr.expr.l);
    }

    // swap sub-expressions if right side is a negation
    if (IS_TYPE(NEG, r)) {
      return update_expr(constr, r->constr.expr.l, r->type->norm(update_unary_expr(r, l)));
    }
  }

  if (is_const(r)) {

    // move constant of addition on left side to right side
    if (IS_TYPE(ADD, l) && is_const(l->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(NEG, l->constr.expr.r, NULL);
      c = l->type->norm(update_expr(l, r, c));

      return update_expr(constr, l->constr.expr.l, c);
    }

    // swap sub-expressions if left side is a negation
    if (IS_TYPE(NEG, l)) {
      return update_expr(constr, l->type->norm(update_unary_expr(l, r)), l->constr.expr.l);
    }
  }

  return update_expr(constr, l, r);
}

// normalize an arithmetic expression (ADD, MUL)
static struct constr_t *normal_arith(struct constr_t *constr, const struct constr_type_t *type, domain_t neutral) {
  NORM_EVAL(constr);

  // normalize sub-expressions
  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  // swap constants to right side
  if (is_const(l)) {
    return update_expr(constr, r, l);
  }

  // reduce to right side if left side is the neutral element
  if (is_const(r) && get_lo(r->constr.term.val) == neutral) {
    return l;
  }

  // swap around expressions if right side is same operation with constant
  if (r->type == type && is_const(r->constr.expr.r)) {
    return update_expr(constr, update_expr(r, l, r->constr.expr.l), r->constr.expr.r);
  }

  // swap around expressions if left side is same operation with constant
  if (l->type == type && is_const(l->constr.expr.r)) {
    return update_expr(constr, l->constr.expr.l, update_expr(l, r, l->constr.expr.r));
  }

  return update_expr(constr, l, r);
}

// normalize addition expression
struct constr_t *normal_add(struct constr_t *constr) {
  return normal_arith(constr, &CONSTR_ADD, 0);
}

// normalize multiplication expression
struct constr_t *normal_mul(struct constr_t *constr) {
  return normal_arith(constr, &CONSTR_MUL, 1);
}

// normalize a unary expression (NEG, NOT)
static struct constr_t *normal_unary(struct constr_t *constr, const struct constr_type_t *type) {
  NORM_EVAL(constr);

  // normalize sub-expressions
  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);

  // reduce expression if sub-expression is the same operation
  if (l->type == type) {
    return l->constr.expr.l;
  }

  return update_unary_expr(constr, l);
}

// normalize negation expression
struct constr_t *normal_neg(struct constr_t *constr) {
  return normal_unary(constr, &CONSTR_NEG);
}

// normalize logical not expression
struct constr_t *normal_not(struct constr_t *constr) {
  return normal_unary(constr, &CONSTR_NOT);
}

// normalize a logic expression (AND, OR)
static struct constr_t *normal_logic(struct constr_t *constr, bool (*is_neutral)(struct val_t), const struct constr_type_t *invtype) {
  NORM_EVAL(constr);

  // normalize sub-expressions
  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  // shortcut if both sides are the same
  if (l == r) {
    return l;
  }

  // reduce to right side if left side is the neutral element
  if (IS_TYPE(TERM, l) && is_neutral(l->constr.term.val)) {
    return r;
  }

  // reduce to left side if right side is the neutral element
  if (IS_TYPE(TERM, r) && is_neutral(r->constr.term.val)) {
    return l;
  }

  // apply DeMorgan's law
  if (IS_TYPE(NOT, l) && IS_TYPE(NOT, r)) {
    struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
    *c = (struct constr_t){
      .type = invtype,
      .constr = { .expr = { .l = l->constr.expr.l,
                            .r = r->constr.expr.l } } };

    return update_unary_expr(l, c);
  }

  return update_expr(constr, l, r);
}

// normalize logical and expression
struct constr_t *normal_and(struct constr_t *constr) {
  return normal_logic(constr, is_true, &CONSTR_OR);
}

// normalize logical or expression
struct constr_t *normal_or(struct constr_t *constr) {
  return normal_logic(constr, is_false, &CONSTR_AND);
}

// normalize wide-and expression
struct constr_t *normal_wand(struct constr_t *constr) {
  struct constr_t *retval = constr;

  // patch sub-expressions of wide-and if they could be normalized
  for (size_t i = 0, l = constr->constr.wand.length; i < l; i++) {
    struct constr_t *o = constr->constr.wand.elems[i].constr;
    struct constr_t *c = o->type->norm(o);
    if (c != o) {
      patch(&retval->constr.wand.elems[i], c);
    }
  }

  return retval;
}

// normalize conflict expression
struct constr_t *normal_confl(struct constr_t *constr) {
  NORM_EVAL(constr);

  return constr;
}

// normalize expression
struct constr_t *normalize(struct constr_t *constr) {
  struct constr_t *retval = constr;
  struct constr_t *prev;
  // normalize as long as return value changes or any wide-and
  // sub-expressions are being patched
  do {
    _patch_count = 0;
    prev = retval;
    retval = retval->type->norm(retval);
  } while (retval != prev || _patch_count > 0);
  return retval;
}
