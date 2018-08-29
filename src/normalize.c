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

#define NORM_EVAL(CONSTR)                       \
  {                                             \
    struct constr_t *e = normal_eval(constr);   \
    if (e != constr) {                          \
      return e;                                 \
    }                                           \
  }

static int32_t _patch_count = 0;

static struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l != constr->constr.expr.l || r != constr->constr.expr.r) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.l = l;
    retval->constr.expr.r = r;
    return retval;
  }
  return constr;
}

static struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  if (l != constr->constr.expr.l) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.l = l;
    retval->constr.expr.r = NULL;
    return retval;
  }
  return constr;
}

struct constr_t *normal_eval(struct constr_t *constr) {
  struct val_t val = constr->type->eval(constr);
  if (is_value(val)) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    *retval = CONSTRAINT_TERM((struct val_t *)alloc(sizeof(struct val_t)));
    *retval->constr.term = val;
    return retval;
  }
  return constr;
}

struct constr_t *normal_term(struct constr_t *constr) {
  return constr;
}

struct constr_t *normal_eq(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  return update_expr(constr, l, r);
}

struct constr_t *normal_lt(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  if (IS_TYPE(NEG, l) && IS_TYPE(NEG, r)) {
    return update_expr(constr, r->constr.expr.l, l->constr.expr.l);
  }

  if (is_const(l)) {

    if (IS_TYPE(ADD, r) && is_const(r->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(NEG, r->constr.expr.r, NULL);
      c = r->type->norm(update_expr(r, l, c));

      return update_expr(constr, c, r->constr.expr.l);
    }

    if (IS_TYPE(NEG, r)) {
      return update_expr(constr, r->constr.expr.l, r->type->norm(update_unary_expr(r, l)));
    }
  }

  if (is_const(r)) {

    if (IS_TYPE(ADD, l) && is_const(l->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(NEG, l->constr.expr.r, NULL);
      c = l->type->norm(update_expr(l, r, c));

      return update_expr(constr, l->constr.expr.l, c);
    }

    if (IS_TYPE(NEG, l)) {
      return update_expr(constr, l->type->norm(update_unary_expr(l, r)), l->constr.expr.l);
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_neg(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);

  if (IS_TYPE(NEG, l)) {
    return l->constr.expr.l;
  }

  return update_unary_expr(constr, l);
}

struct constr_t *normal_add(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  if (is_const(l)) {
    return update_expr(constr, r, l);
  }

  if (is_const(r) && get_lo(*r->constr.term) == 0) {
    return l;
  }

  if (IS_TYPE(ADD, r) && is_const(r->constr.expr.r)) {
    return update_expr(constr, update_expr(r, l, r->constr.expr.l), r->constr.expr.r);
  }

  if (IS_TYPE(ADD, l) && is_const(l->constr.expr.r)) {
    return update_expr(constr, l->constr.expr.l, update_expr(l, r, l->constr.expr.r));
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_mul(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  if (is_const(l)) {
    return update_expr(constr, r, l);
  }

  if (is_const(r) && get_lo(*r->constr.term) == 1) {
    return l;
  }

  if (IS_TYPE(MUL, r) && is_const(r->constr.expr.r)) {
    return update_expr(constr, update_expr(r, l, r->constr.expr.l), r->constr.expr.r);
  }

  if (IS_TYPE(MUL, l) && is_const(l->constr.expr.r)) {
    return update_expr(constr, l->constr.expr.l, update_expr(l, r, l->constr.expr.r));
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_not(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);

  if (IS_TYPE(NOT, l)) {
    return l->constr.expr.l;
  }
  return update_unary_expr(constr, l);
}

struct constr_t *normal_and(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  if (IS_TYPE(TERM, l) && is_true(*l->constr.term)) {
    return r;
  }

  if (IS_TYPE(TERM, r) && is_true(*r->constr.term)) {
    return l;
  }

  if (IS_TYPE(NOT, l) && IS_TYPE(NOT, r)) {

    struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
    *c = CONSTRAINT_EXPR(OR, l->constr.expr.l, r->constr.expr.l);

    return update_unary_expr(l, c);
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_or(struct constr_t *constr) {
  NORM_EVAL(constr);

  struct constr_t *l = constr->constr.expr.l;
  l = l->type->norm(l);
  struct constr_t *r = constr->constr.expr.r;
  r = r->type->norm(r);

  if (IS_TYPE(TERM, l) && is_false(*l->constr.term)) {
    return r;
  }

  if (IS_TYPE(TERM, r) && is_false(*r->constr.term)) {
    return l;
  }

  if (IS_TYPE(NOT, l) && IS_TYPE(NOT, r)) {

    struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
    *c = CONSTRAINT_EXPR(AND, l->constr.expr.l, r->constr.expr.l);

    return update_unary_expr(l, c);
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_wand(struct constr_t *constr) {
  struct constr_t *retval = constr;

  for (size_t i = 0; i < constr->constr.wand.length; i++) {
    struct constr_t *o = constr->constr.wand.elems[i].constr;
    struct constr_t *c = o->type->norm(o);
    if (c != o) {
      patch(&retval->constr.wand.elems[i], (struct wand_expr_t){ c, 0 });
    }
  }

  return retval;
}

struct constr_t *normalize(struct constr_t *constr) {
  struct constr_t *retval = constr;
  struct constr_t *prev;
  do {
    _patch_count = 0;
    prev = retval;
    retval = retval->type->norm(retval);
  } while (retval != prev || _patch_count > 0);
  return retval;
}
