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
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "csolve.h"

// memory allocation functions
#define ALLOC_ALIGNMENT 8

char *_alloc_stack;
size_t _alloc_stack_size;
size_t _alloc_stack_pointer;
size_t alloc_max;

void alloc_init(size_t size) {
  _alloc_stack_size = size;
  _alloc_stack_pointer = 0;
  alloc_max = 0;

  _alloc_stack = (char *)malloc(_alloc_stack_size);
  if (_alloc_stack == 0) {
    print_error("%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void alloc_free(void) {
  free(_alloc_stack);
  _alloc_stack = NULL;
  _alloc_stack_size = 0;
}

void *alloc(size_t size) {
  // align size
  size_t sz = (size + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  // allocate memory
  if (_alloc_stack_pointer + sz < _alloc_stack_size) {
    void *retval = (void *)&_alloc_stack[_alloc_stack_pointer];
    _alloc_stack_pointer += sz;
    if (_alloc_stack_pointer > alloc_max) {
      alloc_max = _alloc_stack_pointer;
    }
    return retval;
  } else {
    print_error(ERROR_MSG_OUT_OF_MEMORY);
    return NULL;
  }
}

void dealloc(void *elem) {
  size_t pointer = (char *)elem - &_alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 &&
      pointer >= 0 && pointer <= _alloc_stack_pointer) {
    _alloc_stack_pointer = pointer;
  } else {
    print_error(ERROR_MSG_WRONG_DEALLOC);
  }
}

// functions to bind (and unbind) variables
struct binding_t *_bind_stack;
size_t _bind_stack_size;
size_t _bind_depth;

void bind_init(size_t size) {
  _bind_stack_size = size;
  _bind_depth = 0;

  _bind_stack = (struct binding_t *)malloc(sizeof(struct binding_t) * _bind_stack_size);
  if (_bind_stack == 0) {
    print_error("%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void bind_free(void) {
  free(_bind_stack);
  _bind_stack = NULL;
  _bind_stack_size = 0;
}

size_t bind(struct val_t *loc, const struct val_t val) {
  if (loc != NULL) {
    if (_bind_depth < _bind_stack_size) {
      _bind_stack[_bind_depth].loc = loc;
      _bind_stack[_bind_depth].val = *loc;
      *loc = val;
      eval_cache_invalidate();
      return _bind_depth++;
    } else {
      print_error(ERROR_MSG_TOO_MANY_BINDS);
      return _bind_stack_size;
    }
  } else {
    print_error(ERROR_MSG_NULL_BIND);
    return _bind_stack_size;
  }
}

void unbind(size_t depth) {
  while (_bind_depth > depth) {
    --_bind_depth;
    struct val_t *loc = _bind_stack[_bind_depth].loc;
    *loc = _bind_stack[_bind_depth].val;
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
    retval->eval_cache.tag = 0;
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
    retval->eval_cache.tag = 0;
    return retval;
  }
  return constr;
}

// functions for semaphores
void sema_init(sem_t *sema) {
  int status = sem_init(sema, 1, 1);
  if (status == -1) {
    print_error("%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void sema_wait(sem_t *sema) {
  int status = sem_wait(sema);
  if (status == -1) {
    print_error("%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}

void sema_post(sem_t *sema) {
  int status = sem_post(sema);
  if (status == -1) {
    print_error("%s", strerror(errno));
    exit(EXIT_FAILURE);
  }
}
