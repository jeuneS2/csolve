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

static int32_t _patch_count = 0;

static struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l != constr->constr.expr.l || r != constr->constr.expr.r) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = constr->type;
    retval->constr.expr.op = constr->constr.expr.op;
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
    retval->constr.expr.op = constr->constr.expr.op;
    retval->constr.expr.l = l;
    retval->constr.expr.r = NULL;
    return retval;
  }
  return constr;
}

struct constr_t *normal(struct constr_t *constr);

struct constr_t *normal_eval(struct constr_t *constr) {
  struct val_t val = eval(constr);
  if (is_value(val)) {
    struct constr_t *retval = (struct constr_t *)alloc(sizeof(struct constr_t));
    retval->type = CONSTR_TERM;
    retval->constr.term = (struct val_t *)alloc(sizeof(struct val_t));
    *retval->constr.term = val;
    return retval;
  }
  return constr;
}

struct constr_t *normal_eq(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);
  return update_expr(constr, l, r);
}

struct constr_t *normal_lt(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);

  if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NEG &&
      r->type == CONSTR_EXPR && r->constr.expr.op == OP_NEG) {
    return update_expr(constr, r->constr.expr.l, l->constr.expr.l);
  }

  if (is_const(l)) {

    if (r->type == CONSTR_EXPR &&
        r->constr.expr.op == OP_ADD &&
        is_const(r->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(OP_NEG, r->constr.expr.r, NULL);
      c = normal(update_expr(r, l, c));

      return update_expr(constr, c, r->constr.expr.l);
    }

    if (r->type == CONSTR_EXPR && r->constr.expr.op == OP_NEG) {
      return update_expr(constr, r->constr.expr.l, normal(update_unary_expr(r, l)));
    }
  }

  if (is_const(r)) {

    if (l->type == CONSTR_EXPR &&
        l->constr.expr.op == OP_ADD &&
        is_const(l->constr.expr.r)) {

      struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
      *c = CONSTRAINT_EXPR(OP_NEG, l->constr.expr.r, NULL);
      c = normal(update_expr(l, r, c));

      return update_expr(constr, l->constr.expr.l, c);
    }

    if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NEG) {
      return update_expr(constr, normal(update_unary_expr(l, r)), l->constr.expr.l);
    }
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_neg(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NEG) {
    return l->constr.expr.l;
  }
  return update_unary_expr(constr, l);
}

struct constr_t *normal_add(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);

  if (is_const(l)) {
    return update_expr(constr, r, l);
  }

  if (is_const(r) && get_lo(*r->constr.term) == 0) {
    return l;
  }

  if (r->type == CONSTR_EXPR &&
      r->constr.expr.op == OP_ADD &&
      is_const(r->constr.expr.r)) {
    return update_expr(constr, update_expr(r, l, r->constr.expr.l), r->constr.expr.r);
  }

  if (l->type == CONSTR_EXPR &&
      l->constr.expr.op == OP_ADD &&
      is_const(l->constr.expr.r)) {
    return update_expr(constr, l->constr.expr.l, update_expr(l, r, l->constr.expr.r));
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_mul(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);

  if (is_const(l)) {
    return update_expr(constr, r, l);
  }

  if (is_const(r) && get_lo(*r->constr.term) == 1) {
    return l;
  }

  if (r->type == CONSTR_EXPR &&
      r->constr.expr.op == OP_MUL &&
      is_const(r->constr.expr.r)) {
    return update_expr(constr, update_expr(r, l, r->constr.expr.l), r->constr.expr.r);
  }

  if (l->type == CONSTR_EXPR &&
      l->constr.expr.op == OP_MUL &&
      is_const(l->constr.expr.r)) {
    return update_expr(constr, l->constr.expr.l, update_expr(l, r, l->constr.expr.r));
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_not(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NOT) {
    return l->constr.expr.l;
  }
  return update_unary_expr(constr, l);
}

struct constr_t *normal_and(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);

  if (is_term(l) && is_true(*l->constr.term)) {
    return r;
  }

  if (is_term(r) && is_true(*r->constr.term)) {
    return l;
  }

  if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NOT &&
      r->type == CONSTR_EXPR && r->constr.expr.op == OP_NOT) {

    struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
    *c = CONSTRAINT_EXPR(OP_OR, l->constr.expr.l, r->constr.expr.l);

    return update_unary_expr(l, c);
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_or(struct constr_t *constr) {
  struct constr_t *l = normal(constr->constr.expr.l);
  struct constr_t *r = normal(constr->constr.expr.r);

  if (is_term(l) && is_false(*l->constr.term)) {
    return r;
  }

  if (is_term(r) && is_false(*r->constr.term)) {
    return l;
  }

  if (l->type == CONSTR_EXPR && l->constr.expr.op == OP_NOT &&
      r->type == CONSTR_EXPR && r->constr.expr.op == OP_NOT) {

    struct constr_t *c = (struct constr_t *)alloc(sizeof(struct constr_t));
    *c = CONSTRAINT_EXPR(OP_AND, l->constr.expr.l, r->constr.expr.l);

    return update_unary_expr(l, c);
  }

  return update_expr(constr, l, r);
}

struct constr_t *normal_wand(struct constr_t *constr) {
  struct constr_t *retval = constr;
  
  for (size_t i = 0; i < constr->constr.wand.length; i++) {
    if (!cache_is_dirty(constr->constr.wand.elems[i].cache_tag)) {
      continue;
    }
    struct constr_t *c = normal(constr->constr.wand.elems[i].constr);
    if (c != constr->constr.wand.elems[i].constr) {
      cache_tag_t t;
      if (is_term(c)) {
        t = 0;
      } else {
        t = constr->constr.wand.elems[i].cache_tag;
      }
      patch(&retval->constr.wand.elems[i], (struct wand_expr_t){ c, t });
    }
  }

  return retval;
}

struct constr_t *normal(struct constr_t *constr) {
  if (constr->type == CONSTR_EXPR) {
    // merge constant values into new terminal node
    struct constr_t *e = normal_eval(constr);
    if (e != constr) {
      return e;
    }
    // handle operation-specific normalization
    switch (constr->constr.expr.op) {
    case OP_EQ:  return normal_eq(constr);
    case OP_LT:  return normal_lt(constr);
    case OP_NEG: return normal_neg(constr);
    case OP_ADD: return normal_add(constr);
    case OP_MUL: return normal_mul(constr);
    case OP_NOT: return normal_not(constr);
    case OP_AND: return normal_and(constr);
    case OP_OR : return normal_or(constr);
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
  } else if (constr->type == CONSTR_WAND) {
    return normal_wand(constr);
  }

  return constr;
}

struct constr_t *normalize(struct constr_t *constr) {
  struct constr_t *retval = constr;
  struct constr_t *prev;
  do {
    _patch_count = 0;
    prev = retval;
    retval = normal(retval);    
  } while (retval != prev || _patch_count > 0);
  return retval;
}
