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

static char *_alloc_stack;
static size_t _alloc_stack_size;
static size_t _alloc_stack_pointer;

void alloc_init(size_t size) {
  _alloc_stack_size = size;
  _alloc_stack_pointer = 0;

  _alloc_stack = (char *)malloc(_alloc_stack_size);
  if (_alloc_stack == 0) {
    print_fatal("%s", strerror(errno));
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
    stat_max_alloc_max(_alloc_stack_pointer);
    return retval;
  } else {
    print_fatal(ERROR_MSG_OUT_OF_MEMORY);
  }
  return NULL;
}

void dealloc(void *elem) {
  size_t pointer = (char *)elem - &_alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 &&
      pointer >= 0 && pointer <= _alloc_stack_pointer) {
    _alloc_stack_pointer = pointer;
  } else {
    print_fatal(ERROR_MSG_WRONG_DEALLOC);
  }
}

// caching functions
cache_tag_t hash_var(const struct val_t *loc) {
  const size_t size = (sizeof(struct val_t) + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  const size_t mod = 8 * sizeof(cache_tag_t) - 1;
  return 1ULL << ((((char *)loc - _alloc_stack) / size) & mod);
}

static cache_tag_t _cache_dirty = -1;

void cache_dirty(cache_tag_t tag) {
  _cache_dirty |= tag;
}

void cache_clean(void) {
  _cache_dirty = 0;
}

bool cache_is_dirty(cache_tag_t tag) {
  return (_cache_dirty & tag) != 0;
}

// functions to bind (and unbind) variables
static struct binding_t *_bind_stack;
static size_t _bind_stack_size;
static size_t _bind_depth;

void bind_init(size_t size) {
  _bind_stack_size = size;
  _bind_depth = 0;

  _bind_stack = (struct binding_t *)malloc(sizeof(struct binding_t) * _bind_stack_size);
  if (_bind_stack == 0) {
    print_fatal("%s", strerror(errno));
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
      cache_dirty(hash_var(loc));
      return _bind_depth++;
    } else {
      print_fatal(ERROR_MSG_TOO_MANY_BINDS);
    }
  } else {
    print_fatal(ERROR_MSG_NULL_BIND);
  }
  return _bind_stack_size;
}

void unbind(size_t depth) {
  while (_bind_depth > depth) {
    --_bind_depth;
    struct val_t *loc = _bind_stack[_bind_depth].loc;
    *loc = _bind_stack[_bind_depth].val;
    cache_dirty(hash_var(loc));
  }
}

// functions for semaphores
void sema_init(sem_t *sema) {
  int status = sem_init(sema, 1, 1);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}

void sema_wait(sem_t *sema) {
  int status = sem_wait(sema);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}

void sema_post(sem_t *sema) {
  int status = sem_post(sema);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}
