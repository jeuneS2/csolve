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

// memory allocation functions
#define ALLOC_ALIGNMENT 8
#define ALLOC_STACK_SIZE (64*0x400*0x400)
char alloc_stack[ALLOC_STACK_SIZE];
size_t alloc_stack_pointer = 0;
size_t alloc_max = 0;

void *alloc(size_t size) {
  // align size
  size_t sz = (size + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  // allocate memory
  if (alloc_stack_pointer + sz < ALLOC_STACK_SIZE) {
    void *retval = (void *)&alloc_stack[alloc_stack_pointer];
    alloc_stack_pointer += sz;
    if (alloc_stack_pointer > alloc_max) {
      alloc_max = alloc_stack_pointer;
    }
    return retval;
  } else {
    print_error(ERROR_MSG_OUT_OF_MEMORY);
    return NULL;
  }
}

void dealloc(void *elem) {
  size_t pointer = (char *)elem - &alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 &&
      pointer >= 0 && pointer <= alloc_stack_pointer) {
    alloc_stack_pointer = pointer;
  } else {
    print_error(ERROR_MSG_WRONG_DEALLOC);
  }
}

// functions to bind (and unbind) variables
#define MAX_BINDS 1024
struct binding_t bind_stack[MAX_BINDS];
size_t bind_depth = 0;

size_t bind(struct val_t *loc, const struct val_t val) {
  if (loc != NULL) {
    if (bind_depth < MAX_BINDS) {
      bind_stack[bind_depth].loc = loc;
      bind_stack[bind_depth].val = *loc;
      *loc = val;
      eval_cache_invalidate();
      return bind_depth++;
    } else {
      print_error(ERROR_MSG_TOO_MANY_BINDS);
      return MAX_BINDS;
    }
  } else {
    print_error(ERROR_MSG_NULL_BIND);
    return MAX_BINDS;
  }
}

void unbind(size_t depth) {
  while (bind_depth > depth) {
    --bind_depth;
    struct val_t *loc = bind_stack[bind_depth].loc;
    *loc = bind_stack[bind_depth].val;
    eval_cache_invalidate();
  }
}

// functions to update expression nodes
struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  if (l == NULL || r == NULL) {
    return NULL;
  }
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

struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  if (l == NULL) {
    return NULL;
  }
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
