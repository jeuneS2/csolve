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

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "csolve.h"

// memory allocation alignment
#define ALLOC_ALIGNMENT 8

// the allocation stack
static char *_alloc_stack;
// the total size of the allocation stack
static size_t _alloc_stack_size;
// the current position in the allocation stack
static size_t _alloc_stack_pointer;

// initialize the allocation stack
void alloc_init(size_t size) {
  _alloc_stack_size = size;
  _alloc_stack_pointer = 0;

  _alloc_stack = (char *)malloc(_alloc_stack_size);
  // die if allocation failed
  if (_alloc_stack == NULL) {
    print_fatal("%s", strerror(errno));
  }
}

// free allocation stack memory
void alloc_free(void) {
  free(_alloc_stack);
  _alloc_stack = NULL;
  _alloc_stack_size = 0;
}

// allocate memory of a certain size
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
    // die if running out of space on the allocation stack
    print_fatal(ERROR_MSG_OUT_OF_MEMORY);
  }
  return NULL;
}

// deallocate memory and all elements allocated later
void dealloc(void *elem) {
  size_t pointer = (char *)elem - &_alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 && pointer <= _alloc_stack_pointer) {
    _alloc_stack_pointer = pointer;
  } else {
    // die if trying to deallocate something that was not allocated
    print_fatal(ERROR_MSG_WRONG_DEALLOC);
  }
}

// the binding stack
static struct binding_t *_bind_stack;
// the total size of the binding stack
static size_t _bind_stack_size;
// the current depth of the binding stack
static size_t _bind_depth;
// the binding level (assignment level of solving algorithm)
static size_t _bind_level;

// initialize the binding stack
void bind_init(size_t size) {
  _bind_stack_size = size;
  _bind_depth = 0;
  _bind_level = -1;

  _bind_stack = (struct binding_t *)malloc(sizeof(struct binding_t) * _bind_stack_size);
  // die if allocation failed
  if (_bind_stack == NULL) {
    print_fatal("%s", strerror(errno));
  }
}

// free the binding stack memory
void bind_free(void) {
  free(_bind_stack);
  _bind_stack = NULL;
  _bind_stack_size = 0;
}

// commit bindings (cannot be undone)
void bind_commit(void) {
  _bind_depth = 0;
}

// get the current binding depth
size_t bind_depth(void) {
  return _bind_depth;
}

// set the current binding level
void bind_level_set(size_t level) {
  _bind_level = level;
}

// get the current binding level
size_t bind_level_get(void) {
  return _bind_level;
}

// bind a variable to a value
void bind(struct env_t *var, const struct val_t val, const struct wand_expr_t *clause) {
  if (var != NULL) {
    if (_bind_depth < _bind_stack_size) {
      _bind_stack[_bind_depth].var = var;

      // update value and store binding level
      _bind_stack[_bind_depth].val = var->val->constr.term.val;
      var->val->constr.term.val = val;
      _bind_stack[_bind_depth].level = var->level;
      var->level = _bind_level;

      // record clause that caused binding and link to previous bindings
      _bind_stack[_bind_depth].clause = clause;
      _bind_stack[_bind_depth].prev = var->binds;
      var->binds = &_bind_stack[_bind_depth];

      _bind_depth++;
    } else {
      // die if running out of space on the bind stack
      print_fatal(ERROR_MSG_TOO_MANY_BINDS);
    }
  } else {
    // die if trying to bind a null variable
    print_fatal(ERROR_MSG_NULL_BIND);
  }
}

// unbind all locations above a certain depth
void unbind(size_t depth) {
  while (_bind_depth > depth) {
    --_bind_depth;
    struct env_t *var = _bind_stack[_bind_depth].var;
    var->val->constr.term.val = _bind_stack[_bind_depth].val;
    var->level = _bind_stack[_bind_depth].level;
    var->binds = _bind_stack[_bind_depth].prev;
  }
}

// the patching stack
static struct patching_t *_patch_stack;
// the total size of the patching stack
static size_t _patch_stack_size;
// the current depth of the patching stack
static size_t _patch_depth;

// initialize patching stack
void patch_init(size_t size) {
  _patch_stack_size = size;
  _patch_depth = 0;

  _patch_stack = (struct patching_t *)malloc(sizeof(struct patching_t) * _patch_stack_size);
  // die if allocation failed
  if (_patch_stack == NULL) {
    print_fatal("%s", strerror(errno));
  }
}

// free patching stack memory
void patch_free(void) {
  free(_patch_stack);
  _patch_stack = NULL;
  _patch_stack_size = 0;
}

// commit patches (cannot be undone)
void patch_commit(void) {
  _patch_depth = 0;
}

// patch the constraint at a location
size_t patch(struct wand_expr_t *loc, struct constr_t *constr) {
  if (loc != NULL) {
    if (_patch_depth < _patch_stack_size) {
      _patch_stack[_patch_depth].loc = loc;
      _patch_stack[_patch_depth].constr = loc->constr;
      loc->constr = constr;
      return _patch_depth++;
    } else {
    // die if running out of space on the patching stack
      print_fatal(ERROR_MSG_TOO_MANY_PATCHES);
    }
  } else {
    return _patch_depth;
  }
  return _patch_stack_size;
}

// unpatch locations above a certain depth
void unpatch(size_t depth) {
  while (_patch_depth > depth) {
    --_patch_depth;
    struct wand_expr_t *loc = _patch_stack[_patch_depth].loc;
    loc->constr = _patch_stack[_patch_depth].constr;
  }
}

// initialize a semaphore
void sema_init(sem_t *sema) {
  int status = sem_init(sema, 1, 1);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}

// wait for a semaphore
void sema_wait(sem_t *sema) {
  int status = sem_wait(sema);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}

// release a semaphore
void sema_post(sem_t *sema) {
  int status = sem_post(sema);
  if (status == -1) {
    print_fatal("%s", strerror(errno));
  }
}

// check whether a clause list already contains an element
bool clause_list_contains(struct clause_list_t *list, struct wand_expr_t *elem) {
  for (size_t i = 0, l = list->length; i < l; i++) {
    if (list->elems[i] == elem) {
      return true;
    }
  }
  return false;
}

// add an element to a clause list
void clause_list_append(struct clause_list_t *list, struct wand_expr_t *elem) {
  // only add element if it is not already on the list
  if (!clause_list_contains(list, elem)) {
    list->length++;
    list->elems = (struct wand_expr_t **)realloc(list->elems, list->length * sizeof(struct wand_expr_t *));
    list->elems[list->length-1] = elem;
  }
}
