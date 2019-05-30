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

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the maximum assignment level in this conflict
static size_t _conflict_max_level;
// the assignment level where the conflict should be resolved
static size_t _conflict_level;
// the conflicting variable
static struct env_t *_conflict_var;

// definition for return value of conflict-creating functions
typedef bool confl_result_t;
// conflict creation failed
#define CONFL_ERROR false
// conflicted creation went ok
#define CONFL_OK    true

// return if conflict creation resulted in an error
#define CHECK(VAR)          \
  if ((VAR) == CONFL_ERROR) {                   \
    return CONFL_ERROR;     \
  }

// maximum length of array of already seen conflict elements
#define SEEN_ARRAY_LENGTH_MAX 1024
// type to hold already seen conflict elements
struct seen_array_t {
  size_t length;
  void *elems[SEEN_ARRAY_LENGTH_MAX];
};

// breadth of array of already seen conflict elements
#define SEEN_ARRAY_WIDTH 64
// array of already seen conflict elements
static struct seen_array_t _seen[SEEN_ARRAY_WIDTH];

// conflict memory allocation alignment
#define ALLOC_ALIGNMENT 8
// the conflict allocation stack
static char *_alloc_stack;
// the total size of the conflict allocation stack
static size_t _alloc_stack_size;
// the current position in the conflict allocation stack
static size_t _alloc_stack_pointer;

// initialize the conflict allocation stack
void conflict_alloc_init(size_t size) {
  _alloc_stack_size = size;
  _alloc_stack_pointer = 0;

  _alloc_stack = (char *)malloc(_alloc_stack_size);
  // die if allocation failed
  if (_alloc_stack == 0) {
    print_fatal("%s", strerror(errno));
  }
}

// free conflict allocation stack memory
void conflict_alloc_free(void) {
  free(_alloc_stack);
  _alloc_stack = NULL;
  _alloc_stack_size = 0;
}

// allocate conflict memory of a certain size
static void *conflict_alloc(void *ptr, size_t size) {
  // get new pointer or reuse
  char *p = ptr == NULL ? &_alloc_stack[_alloc_stack_pointer] : (char *)ptr;
  // align size
  size_t sz = (size + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  // allocate memory
  if (&p[sz] < &_alloc_stack[_alloc_stack_size]) {
    _alloc_stack_pointer = &p[sz] - &_alloc_stack[0];
    stat_max_calloc_max(_alloc_stack_pointer);
    return (void *)p;
  }

  // die if running out of space on the conflict allocation stack
  print_fatal(ERROR_MSG_OUT_OF_MEMORY);
  return NULL;
}

// deallocate conflict memory and all elements allocated later
static void conflict_dealloc(void *ptr) {
  size_t pointer = (char *)ptr - &_alloc_stack[0];
  if ((pointer & (ALLOC_ALIGNMENT-1)) == 0 && pointer <= _alloc_stack_pointer) {
    _alloc_stack_pointer = pointer;
  } else {
    // die if trying to deallocate something that was not allocated
    print_fatal(ERROR_MSG_WRONG_DEALLOC);
  }
}

// forward declaration
static confl_result_t conflict_add_var(struct env_t *var, struct constr_t *confl);

// get the current conflict level
size_t conflict_level(void) {
  return _conflict_level;
}

// get the curent conflict variable
struct env_t *conflict_var(void) {
  return _conflict_var;
}

// reset the conflict
void conflict_reset(void) {
  _conflict_level = SIZE_MAX;
  _conflict_var = NULL;
}

// reset the information about seen conflict elements
static void conflict_seen_reset(void) {
  for (size_t i = 0; i < SEEN_ARRAY_WIDTH; i++) {
    _seen[i].length = 0;
  }
}

// calculate hash for a conflict element
static uint32_t conflict_seen_hash(void *elem) {
  return (intptr_t)elem / max(sizeof(struct confl_elem_t) + sizeof(size_t),
                              sizeof(struct env_t));
}

// check whether a conflict element already has been seen
static bool conflict_seen(void *elem) {
  struct seen_array_t *arr = &_seen[conflict_seen_hash(elem) % SEEN_ARRAY_WIDTH];
  for (size_t i = 0, l = arr->length; i < l; i++) {
    if (arr->elems[i] == elem) {
      return true;
    }
  }
  return false;
}

// add a conflict element to the array of already seen conflict elements
static confl_result_t conflict_seen_add(void *elem) {
  struct seen_array_t *arr = &_seen[conflict_seen_hash(elem) % SEEN_ARRAY_WIDTH];
  arr->length++;
  // check if there is space in the array of seen conflict elements
  if (arr->length > SEEN_ARRAY_LENGTH_MAX) {
    return CONFL_ERROR;
  }
  arr->elems[arr->length-1] = elem;
  return CONFL_OK;
}

// add a terminal to the conflict
static confl_result_t conflict_add_term(struct constr_t *confl, struct constr_t *constr) {
  // do not generate conflict for non-binary variables
  if (!is_value(constr->constr.term.val) ||
      get_lo(constr->constr.term.val) > 1 ||
      get_lo(constr->constr.term.val) < 0) {
    return CONFL_ERROR;
  }

  // add new element to the conflict expression
  size_t length = ++confl->constr.confl.length;
  const size_t size = length * sizeof(struct confl_elem_t);
  confl->constr.confl.elems = (struct confl_elem_t *)conflict_alloc(confl->constr.confl.elems, size);
  confl->constr.confl.elems[length-1] =
    (struct confl_elem_t) { .val = constr->constr.term.val, .var = constr };

  // update maximum conflict level
  size_t level = constr->constr.term.env->level;
  if (level > _conflict_max_level) {
    _conflict_max_level = level;
  }

  return CONFL_OK;
}

// forward declaration
static confl_result_t conflict_add_constr(struct env_t *var, struct constr_t *confl, struct constr_t *constr);

// process terminal when adding a constraint to the conflict
static confl_result_t conflict_add_constr_term(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  // add terminal if it is a variable and different from the currently processed one
  if (constr->constr.term.env != NULL && constr->constr.term.env != var) {
    if (constr->constr.term.env->level < bind_level_get()
        || (constr->constr.term.env->binds != NULL
            && constr->constr.term.env->binds->clause == NULL)) {
      // add terminal if bound at lower level or bound without being inferred
      return conflict_add_term(confl, constr);
    }
    // otherwise, add the variable to the conflict
    return conflict_add_var(constr->constr.term.env, confl);
  }
  return CONFL_OK;
}

// process wide-and expression when adding a constraint to the conflict
static confl_result_t conflict_add_constr_wand(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  // add all sub-expressions of wide-and expression
  for (size_t i = 0, l = constr->constr.wand.length; i < l; i++) {
    confl_result_t c = conflict_add_constr(var, confl, constr->constr.wand.elems[i].constr);
    CHECK(c);
  }
  return CONFL_OK;
}

// process conflict expression when adding a constraint to the conflict
static confl_result_t conflict_add_constr_confl(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  // add all variables in conflict expression
  for (size_t i = 0, l = constr->constr.confl.length; i < l; i++) {
    confl_result_t c = conflict_add_constr(var, confl, constr->constr.confl.elems[i].var);
    CHECK(c);
  }
  return CONFL_OK;
}

// process ordinary expression when adding a constraint to the conflict
static confl_result_t conflict_add_constr_expr(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  switch (constr->type->op) {
  case OP_EQ:
  case OP_LT:
  case OP_ADD:
  case OP_MUL:
  case OP_AND:
  case OP_OR: {
    // add right sub-expression
    confl_result_t c = conflict_add_constr(var, confl, constr->constr.expr.r);
    CHECK(c);
    /* fall through */
  }
  case OP_NEG:
  case OP_NOT: {
    // add left sub-expression
    confl_result_t c = conflict_add_constr(var, confl, constr->constr.expr.l);
    CHECK(c);
    break;
  }
  default:
    print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
  }

  return CONFL_OK;
}

// add a constraint to the conflict
static confl_result_t conflict_add_constr(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  // only process constraints that have not been seen yet
  if (conflict_seen(constr)) {
    return CONFL_OK;
  }
  confl_result_t a = conflict_seen_add(constr);
  CHECK(a);

  if (IS_TYPE(TERM, constr)) {
    return conflict_add_constr_term(var, confl, constr);
  }
  if (IS_TYPE(WAND, constr)) {
    return conflict_add_constr_wand(var, confl, constr);
  }
  if (IS_TYPE(CONFL, constr)) {
    return conflict_add_constr_confl(var, confl, constr);
  }
  return conflict_add_constr_expr(var, confl, constr);
}

// add a variable to the conflict
static confl_result_t conflict_add_var(struct env_t *var, struct constr_t *confl) {
  // only process variables that have not been seen yet
  if (conflict_seen(var)) {
    return CONFL_OK;
  }
  confl_result_t a = conflict_seen_add(var);
  CHECK(a);

  // iterate over all the bindings of the variable
  for (struct binding_t *b = var->binds; b != NULL; b = b->prev) {
    if (b->clause != NULL) {
      // add constraint if binding was inferred by a clause
      confl_result_t c = conflict_add_constr(var, confl, b->clause->orig);
      CHECK(c);
    } else {
      // add terminal if binding was not inferred
      confl_result_t c = conflict_add_term(confl, b->var->val);
      CHECK(c);
    }
  }

  return CONFL_OK;
}

// update the conflict level and conflict variable
static void conflict_update(struct constr_t *confl) {
  if (confl->constr.confl.length != 0) {
    _conflict_level = 0;
    _conflict_var = confl->constr.confl.elems[0].var->constr.term.env;
    // find the level where the conflict can be resolved
    for (size_t i = 0, l = confl->constr.confl.length; i < l; i++) {
      size_t level = confl->constr.confl.elems[i].var->constr.term.env->level;
      if (level < _conflict_max_level && level+1 > _conflict_level) {
        _conflict_level = level+1;
        _conflict_var = confl->constr.confl.elems[i].var->constr.term.env;
      }
    }
  }
}

// create a conflict
void conflict_create(struct env_t *var, const struct wand_expr_t *clause) {
  // allocate a new conflict expression
  struct constr_t *confl = (struct constr_t *)conflict_alloc(NULL, sizeof(struct constr_t));
  *confl = CONSTRAINT_CONFL(0, NULL);

  // reset conflict information
  conflict_seen_reset();
  conflict_reset();
  _conflict_max_level = 0;

  // add the clause that caused the conflict
  confl_result_t c1 = conflict_add_constr(var, confl, clause->orig);
  if (c1 == CONFL_ERROR) {
    conflict_dealloc(confl);
    return;
  }
  // add the variable that caused the conflict
  confl_result_t c2 = conflict_add_var(var, confl);
  if (c2 == CONFL_ERROR) {
    conflict_dealloc(confl);
    return;
  }

  // update conflict level and variable
  conflict_update(confl);

  // add the newly created conflict to the relevant clause lists
  struct wand_expr_t *c = (struct wand_expr_t *)conflict_alloc(NULL, sizeof(struct wand_expr_t));
  *c = (struct wand_expr_t){ .constr = confl, .orig = confl, .prop_tag = 0 };
  for (size_t i = 0, l = confl->constr.confl.length; i < l; i++) {
    clause_list_append(&confl->constr.confl.elems[i].var->constr.term.env->clauses, c);
  }

  // update statistics
  stat_inc_confl();
}
