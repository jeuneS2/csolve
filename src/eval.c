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

const struct val_t eval_term(const struct constr_t *constr) {
  return constr->constr.term.val;
}

const struct val_t eval_eq(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t a = l->type->eval(l);
  const struct val_t b = r->type->eval(r);

  domain_t a_lo = get_lo(a);
  domain_t a_hi = get_hi(a);
  domain_t b_lo = get_lo(b);
  domain_t b_hi = get_hi(b);
  if (a_lo == DOMAIN_MIN || a_hi == DOMAIN_MAX ||
      b_lo == DOMAIN_MIN || b_hi == DOMAIN_MAX) {
    return INTERVAL(0, 1);
  }
  if (a_hi == b_hi && a_lo == b_lo && a_hi == a_lo) {
    return VALUE(1);
  }
  if (a_hi < b_lo || a_lo > b_hi) {
    return VALUE(0);
  }

  return INTERVAL(0, 1);
}

const struct val_t eval_lt(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t a = l->type->eval(l);
  const struct val_t b = r->type->eval(r);

  domain_t a_lo = get_lo(a);
  domain_t a_hi = get_hi(a);
  domain_t b_lo = get_lo(b);
  domain_t b_hi = get_hi(b);
  if (a_lo == DOMAIN_MIN || a_hi == DOMAIN_MAX ||
      b_lo == DOMAIN_MIN || b_hi == DOMAIN_MAX) {
    return INTERVAL(0, 1);
  }
  if (a_hi < b_lo) {
    return VALUE(1);
  }
  if (a_lo >= b_hi) {
    return VALUE(0);
  }

  return INTERVAL(0, 1);
}

const struct val_t eval_neg(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct val_t a = l->type->eval(l);

  domain_t a_lo = get_lo(a);
  domain_t a_hi = get_hi(a);
  domain_t lo = neg(a_hi);
  domain_t hi = neg(a_lo);
  return INTERVAL(lo, hi);
}

const struct val_t eval_add(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t a = l->type->eval(l);
  const struct val_t b = r->type->eval(r);

  domain_t a_lo = get_lo(a);
  domain_t a_hi = get_hi(a);
  domain_t b_lo = get_lo(b);
  domain_t b_hi = get_hi(b);
  domain_t lo = add(a_lo, b_lo);
  domain_t hi = add(a_hi, b_hi);
  return INTERVAL(lo, hi);
}

const struct val_t eval_mul(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t a = l->type->eval(l);
  const struct val_t b = r->type->eval(r);

  domain_t a_lo = get_lo(a);
  domain_t a_hi = get_hi(a);
  domain_t b_lo = get_lo(b);
  domain_t b_hi = get_hi(b);
  domain_t ll = mul(a_lo, b_lo);
  domain_t lh = mul(a_lo, b_hi);
  domain_t hl = mul(a_hi, b_lo);
  domain_t hh = mul(a_hi, b_hi);
  domain_t lo = min(min(ll, lh), min(hl, hh));
  domain_t hi = max(max(ll, lh), max(hl, hh));
  return INTERVAL(lo, hi);
}

const struct val_t eval_not(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct val_t a = l->type->eval(l);

  if (is_true(a)) {
    return VALUE(0);
  }
  if (is_false(a)) {
    return VALUE(1);
  }
  return INTERVAL(0, 1);
}

const struct val_t eval_and(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct val_t lval = l->type->eval(l);
  if (is_false(lval)) {
    return VALUE(0);
  }

  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t rval = r->type->eval(r);
  if (is_false(rval)) {
    return VALUE(0);
  }

  if (is_true(lval) && is_true(rval)) {
    return VALUE(1);
  }

  return INTERVAL(0, 1);
}

const struct val_t eval_or(const struct constr_t *constr) {
  const struct constr_t *l = constr->constr.expr.l;
  const struct val_t lval = l->type->eval(l);
  if (is_true(lval)) {
    return VALUE(1);
  }

  const struct constr_t *r = constr->constr.expr.r;
  const struct val_t rval = r->type->eval(r);
  if (is_true(rval)) {
    return VALUE(1);
  }

  if (is_false(lval) && is_false(rval)) {
    return VALUE(0);
  }

  return INTERVAL(0, 1);
}

const struct val_t eval_wand(const struct constr_t *constr) {
  bool all_true = true;

  for (size_t i = 0; i < constr->constr.wand.length; i++) {
    const struct constr_t *c = constr->constr.wand.elems[i].constr;
    struct val_t val = c->type->eval(c);
    if (is_false(val)) {
      return VALUE(0);
    }
    if (!is_true(val)) {
      all_true = false;
    }
  }

  if (all_true) {
    return VALUE(1);
  }

  return INTERVAL(0, 1);
}
