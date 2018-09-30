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
#include <errno.h>
#include "csolve.h"

static size_t _conflict_max_level;
static size_t _conflict_level;
static struct env_t *_conflict_var;


typedef bool confl_result_t;
#define CONFL_ERROR false
#define CONFL_OK    true

#define CHECK(VAR)          \
  if (VAR == CONFL_ERROR) { \
    return CONFL_ERROR;     \
  }


#define SEEN_ARRAY_LENGTH_MAX 1024
struct seen_array_t {
  size_t length;
  void *elems[SEEN_ARRAY_LENGTH_MAX];
};

#define SEEN_ARRAY_WIDTH 64
static struct seen_array_t _seen[SEEN_ARRAY_WIDTH];


#define ALLOC_ALIGNMENT 8
static char *_alloc_stack;
static size_t _alloc_stack_size;
static size_t _alloc_stack_pointer;

void conflict_alloc_init(size_t size) {
  _alloc_stack_size = size;
  _alloc_stack_pointer = 0;

  _alloc_stack = (char *)malloc(_alloc_stack_size);
  if (_alloc_stack == 0) {
    print_fatal("%s", strerror(errno));
  }
}

void conflict_alloc_free(void) {
  free(_alloc_stack);
  _alloc_stack = NULL;
  _alloc_stack_size = 0;
}

void *conflict_alloc(void *ptr, size_t size) {
  // get new pointer or reuse
  char *p = ptr == NULL ? &_alloc_stack[_alloc_stack_pointer] : ptr;
  // align size
  size_t sz = (size + (ALLOC_ALIGNMENT-1)) & ~(ALLOC_ALIGNMENT-1);
  // allocate memory
  if (&p[sz] < &_alloc_stack[_alloc_stack_size]) {
    _alloc_stack_pointer = &p[sz] - &_alloc_stack[0];
    stat_max_calloc_max(_alloc_stack_pointer);
    return (void *)p;
  } else {
    print_fatal(ERROR_MSG_OUT_OF_MEMORY);
  }
  return NULL;
}

confl_result_t conflict_add_var(struct env_t *var, struct constr_t *confl);

size_t conflict_level(void) {
  return _conflict_level;
}

struct env_t *conflict_var(void) {
  return _conflict_var;
}

void conflict_reset(void) {
  _conflict_level = SIZE_MAX;
  _conflict_var = NULL;
}

void conflict_seen_reset(void) {
  for (size_t i = 0; i < SEEN_ARRAY_WIDTH; i++) {
    _seen[i].length = 0;
  }
}

uint32_t conflict_seen_hash(void *elem) {
  return (intptr_t)elem / max(sizeof(struct confl_t), sizeof(struct env_t));
}

bool conflict_seen(void *elem) {
  struct seen_array_t *arr = &_seen[conflict_seen_hash(elem) % SEEN_ARRAY_WIDTH];
  for (size_t i = 0, l = arr->length; i < l; i++) {
    if (arr->elems[i] == elem) {
      return true;
    }
  }
  return false;
}

confl_result_t conflict_seen_add(void *elem) {
  struct seen_array_t *arr = &_seen[conflict_seen_hash(elem) % SEEN_ARRAY_WIDTH];
  arr->length++;
  if (arr->length > SEEN_ARRAY_LENGTH_MAX) {
    return CONFL_ERROR;
  }
  arr->elems[arr->length-1] = elem;
  return CONFL_OK;
}

confl_result_t conflict_add_elem(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  if (!is_value(constr->constr.term.val) ||
      get_lo(constr->constr.term.val) > 1||
      get_lo(constr->constr.term.val) < 0) {
    return CONFL_ERROR;
  }

  size_t length = ++confl->constr.confl.length;
  const size_t size = length * sizeof(struct confl_elem_t);
  confl->constr.confl.elems = conflict_alloc(confl->constr.confl.elems, size);
  confl->constr.confl.elems[length-1] =
    (struct confl_elem_t) { .val = constr->constr.term.val, .var = constr };

  size_t level = constr->constr.term.env->level;
  if (level > _conflict_max_level) {
    _conflict_max_level = level;
  }

  return CONFL_OK;
}

confl_result_t conflict_add_constr(struct env_t *var, struct constr_t *confl, struct constr_t *constr) {
  if (conflict_seen(constr)) {
    return CONFL_OK;
  }
  confl_result_t a = conflict_seen_add(constr);
  CHECK(a);

  if (IS_TYPE(TERM, constr)) {
    if (constr->constr.term.env != NULL && constr->constr.term.env != var) {
      if (constr->constr.term.env->level < bind_level_get()
          || (constr->constr.term.env->binds != NULL
              && constr->constr.term.env->binds->clause == NULL)) {
        confl_result_t c = conflict_add_elem(var, confl, constr);
        CHECK(c);
      } else {
        return conflict_add_var(constr->constr.term.env, confl);
      }
    }
  } else if (IS_TYPE(WAND, constr)) {
    for (size_t i = 0, l = constr->constr.wand.length; i < l; i++) {
      confl_result_t c = conflict_add_constr(var, confl, constr->constr.wand.elems[i].constr);
      CHECK(c);
    }
  } else if (IS_TYPE(CONFL, constr)) {
    for (size_t i = 0, l = constr->constr.confl.length; i < l; i++) {
      confl_result_t c = conflict_add_constr(var, confl, constr->constr.confl.elems[i].var);
      CHECK(c);
    }
  } else {
    switch (constr->type->op) {
    case OP_EQ:
    case OP_LT:
    case OP_ADD:
    case OP_MUL:
    case OP_AND:
    case OP_OR: {
      confl_result_t c = conflict_add_constr(var, confl, constr->constr.expr.r);
      CHECK(c);
      /* fall through */
    }
    case OP_NEG:
    case OP_NOT: {
      confl_result_t c = conflict_add_constr(var, confl, constr->constr.expr.l);
      CHECK(c);
      break;
    }
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
    }
  }
  return CONFL_OK;
}

confl_result_t conflict_add_var(struct env_t *var, struct constr_t *confl) {
  if (conflict_seen(var)) {
    return CONFL_OK;
  }
  confl_result_t a = conflict_seen_add(var);
  CHECK(a);

  for (struct binding_t *b = var->binds; b != NULL; b = b->prev) {
    if (b->clause != NULL) {
      confl_result_t c = conflict_add_constr(var, confl, b->clause->orig);
      CHECK(c);
    } else {
      confl_result_t c = conflict_add_elem(var, confl, b->var->val);
      CHECK(c);
    }
  }

  return CONFL_OK;
}

void conflict_update(struct constr_t *confl) {
  if (confl->constr.confl.length != 0) {
    _conflict_level = 0;
    _conflict_var = confl->constr.confl.elems[0].var->constr.term.env;
    for (size_t i = 0, l = confl->constr.confl.length; i < l; i++) {
      size_t level = confl->constr.confl.elems[i].var->constr.term.env->level;
      if (level < _conflict_max_level && level+1 > _conflict_level) {
        _conflict_level = level+1;
        _conflict_var = confl->constr.confl.elems[i].var->constr.term.env;
      }
    }
  }
}

void conflict_create(struct env_t *var, const struct wand_expr_t *clause) {
  struct constr_t *confl = (struct constr_t *)conflict_alloc(NULL, sizeof(struct constr_t));
  *confl = CONSTRAINT_CONFL(0, NULL);

  conflict_seen_reset();
  conflict_reset();
  _conflict_max_level = 0;

  confl_result_t c1 = conflict_add_constr(var, confl, clause->orig);
  if (c1 == CONFL_ERROR) {
    return;
  }
  confl_result_t c2 = conflict_add_var(var, confl);
  if (c2 == CONFL_ERROR) {
    return;
  }

  conflict_update(confl);

  struct wand_expr_t *c = (struct wand_expr_t *)conflict_alloc(NULL, sizeof(struct wand_expr_t));
  *c = (struct wand_expr_t){ .constr = confl, .orig = confl, .prop_tag = 0 };
  for (size_t i = 0, l = confl->constr.confl.length; i < l; i++) {
    clause_list_append(&confl->constr.confl.elems[i].var->constr.term.env->clauses, c);
  }

  stat_inc_confl();
}
