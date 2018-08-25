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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "csolve.h"
#include "parser_support.h"

static struct env_t *_vars = NULL;
static size_t _var_count = 0;

/** Type to represent a list of variables in a hash table */
struct var_list_t {
  size_t index; ///< Index of variable in _vars
  struct var_list_t *next; ///< Next element in list
};

#define TABLE_SIZE 4096
static struct var_list_t *_keytab[TABLE_SIZE];
static struct var_list_t *_valtab[TABLE_SIZE];

static struct var_list_t *var_list_append(struct var_list_t *list, size_t elem) {
  struct var_list_t *retval = (struct var_list_t *)malloc(sizeof(struct var_list_t));
  retval->index = elem;
  retval->next = list;
  return retval;
}

static void var_list_free(struct var_list_t *list) {
  struct var_list_t *next;
  for (struct var_list_t *l = list; l != NULL; l = next) {
    next = l->next;
    free(l);
  }
}

#define HASH_STR_MAGIC 31
static uint32_t hash_str(const char *str) {
  int32_t hash = 0;
  while (*str != '\0') {
    hash = hash * HASH_STR_MAGIC + *str;
    str++;
  }
  return hash;
}

static void keytab_add(size_t index) {
  size_t hash = hash_str(_vars[index].key) % TABLE_SIZE;
  _keytab[hash] = var_list_append(_keytab[hash], index);
}

static struct env_t *keytab_find(const char *key) {
  size_t index = hash_str(key) % TABLE_SIZE;
  for (struct var_list_t *l = _keytab[index]; l != NULL; l = l->next) {
    if (strcmp(key, _vars[l->index].key) == 0) {
      return &_vars[l->index];
    }
  }
  return NULL;
}

static void keytab_free(void) {
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    var_list_free(_keytab[i]);
    _keytab[i] = NULL;
  }
}

struct env_t *vars_find_key(const char *key) {
  return keytab_find(key);
}

static uint32_t hash_val(const struct val_t *val) {
  return (intptr_t)val / sizeof(struct val_t);
}

static void valtab_add(size_t index) {
  size_t hash = hash_val(_vars[index].val) % TABLE_SIZE;
  _valtab[hash] = var_list_append(_valtab[hash], index);
}

static struct env_t *valtab_find(const struct val_t *val) {
  size_t index = hash_val(val) % TABLE_SIZE;
  for (struct var_list_t *l = _valtab[index]; l != NULL; l = l->next) {
    if (val == _vars[l->index].val) {
      return &_vars[l->index];
    }
  }
  return NULL;
}

static void valtab_free(void) {
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    var_list_free(_valtab[i]);
    _valtab[i] = NULL;
  }
}

struct env_t *vars_find_val(const struct val_t *val) {
  return valtab_find(val);
}

size_t var_count(void) {
  return _var_count;
}

void vars_add(const char *key, struct val_t *val) {
  _var_count++;
  _vars = (struct env_t *)realloc(_vars, sizeof(struct env_t) * _var_count);
  _vars[_var_count-1].key = (const char *)malloc(strlen(key)+1);
  strcpy((char *)_vars[_var_count-1].key, key);
  _vars[_var_count-1].val = val;
  _vars[_var_count-1].clauses = NULL;
  _vars[_var_count-1].order = SIZE_MAX;
  _vars[_var_count-1].prio = 0;

  keytab_add(_var_count-1);
  valtab_add(_var_count-1);
}

int32_t vars_count(struct constr_t *constr) {
  switch (constr->type) {
  case CONSTR_TERM:
    if (is_value(*constr->constr.term)) {
      return 0;
    } else {
      return 1;
    }
  case CONSTR_EXPR:
    switch (constr->constr.expr.op) {
    case OP_EQ:
    case OP_LT:
    case OP_ADD:
    case OP_MUL:
    case OP_AND:
    case OP_OR:
      return vars_count(constr->constr.expr.l) + vars_count(constr->constr.expr.r);
    case OP_NEG:
    case OP_NOT:
      return vars_count(constr->constr.expr.l);
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
  }
  return 0;
}

void vars_weighten(struct constr_t *constr, int32_t weight) {
  switch (constr->type) {
  case CONSTR_TERM:
    if (!is_value(*constr->constr.term)) {
      struct env_t *var = vars_find_val(constr->constr.term);
      var->prio += weight;
    }
    break;
  case CONSTR_EXPR:
    switch (constr->constr.expr.op) {
    case OP_EQ:
    case OP_LT:
    case OP_ADD:
    case OP_MUL:
    case OP_AND:
    case OP_OR:
      vars_weighten(constr->constr.expr.r, weight);
      /* fall through */
    case OP_NEG:
    case OP_NOT:
      vars_weighten(constr->constr.expr.l, weight);
      break;
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
  }
}

struct env_t *env_generate(void) {
  for (size_t i = 0; i < _var_count; i++) {
    _vars[i].val->env = &_vars[i];
  }

  return _vars;
}

void env_free(void) {
  keytab_free();
  valtab_free();

  for (size_t i = 0; i < _var_count; i++) {
    free((char *)_vars[i].key);
  }
  free(_vars);

  _vars = NULL;
  _var_count = 0;
}

struct expr_list_t *expr_list_append(struct expr_list_t *list, struct constr_t *elem) {
  struct expr_list_t *retval = (struct expr_list_t *)malloc(sizeof(struct expr_list_t));
  retval->expr = elem;
  retval->next = list;
  return retval;
}

void expr_list_free(struct expr_list_t *list) {
  struct expr_list_t *next;
  for (struct expr_list_t *l = list; l != NULL; l = next) {
    next = l->next;
    free(l);
  }
}

void clauses_init(struct constr_t *constr, struct wand_expr_t *clause) {
  switch (constr->type) {
  case CONSTR_TERM:
    if (!is_value(*constr->constr.term) && clause != NULL) {
      struct env_t *e = constr->constr.term->env;
      e->clauses = clause_list_append(e->clauses, clause);
    }
    break;
  case CONSTR_EXPR:
    switch (constr->constr.expr.op) {
    case OP_EQ:
    case OP_LT:
    case OP_ADD:
    case OP_MUL:
    case OP_AND:
    case OP_OR:
      clauses_init(constr->constr.expr.r, clause);
      /* fall through */
    case OP_NEG:
    case OP_NOT:
      clauses_init(constr->constr.expr.l, clause);
      break;
    default:
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  case CONSTR_WAND:
    for (size_t i = 0; i < constr->constr.wand.length; i++) {
      struct wand_expr_t *c = clause;
      if (clause == NULL && constr->constr.wand.elems[i].constr->type != CONSTR_WAND) {
        c = &constr->constr.wand.elems[i];
      }
      clauses_init(constr->constr.wand.elems[i].constr, c);
    }
    break;
  default:
    print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
  }
}
