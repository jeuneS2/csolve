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

static struct var_t *_vars = NULL;
static size_t _var_count = 0;

struct var_t *vars_find_key(const char *key) {
  for (size_t i = 0; i < _var_count; i++) {
    if (strcmp(_vars[i].var.key, key) == 0) {
      return &_vars[i];
    }
  }
  return NULL;
}
struct var_t *vars_find_val(const struct val_t *val) {
  for (size_t i = 0; i < _var_count; i++) {
    if (_vars[i].var.val == val) {
      return &_vars[i];
    }
  }
  return NULL;
}

void vars_add(const char *key, struct val_t *val) {
  _var_count++;
  _vars = (struct var_t *)realloc(_vars, sizeof(struct var_t) * _var_count);
  _vars[_var_count-1].var.key = (const char *)malloc(strlen(key)+1);
  strcpy((char *)_vars[_var_count-1].var.key, key);
  _vars[_var_count-1].var.val = val;
  _vars[_var_count-1].var.fails = 0;
  _vars[_var_count-1].var.step = (struct solve_step_t *)calloc(1, sizeof(struct solve_step_t));
  _vars[_var_count-1].weight = 0;
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
      struct var_t *var = vars_find_val(constr->constr.term);
      var->weight += weight;
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

int vars_compare(const void *a, const void *b) {
  const struct var_t *x = (const struct var_t *)a;
  const struct var_t *y = (const struct var_t *)b;
  return y->weight - x->weight;
}

void vars_sort(void) {
  qsort(_vars, _var_count, sizeof(struct var_t), vars_compare);
}

void vars_print(FILE *file) {
  for (size_t i = 0; i < _var_count; i++) {
    fprintf(file, "%s: %d", _vars[i].var.key, _vars[i].weight);
    print_val(file, *_vars[i].var.val);
    fprintf(file, "\n");
  }
}

void vars_free(void) {
  free(_vars);
  _vars = NULL;
  _var_count = 0;
}

struct env_t *env_generate(void) {
  struct env_t *env = (struct env_t *)malloc(sizeof(struct env_t) * (_var_count+1));
  for (size_t i = 0; i < _var_count; i++) {
    env[i] = _vars[i].var;
  }
  env[_var_count].key = NULL;
  env[_var_count].val = NULL;
  env[_var_count].fails = 0;
  env[_var_count].step = (struct solve_step_t *)calloc(1, sizeof(struct solve_step_t));

  vars_free();

  return env;
}

void env_free(struct env_t *env) {
  size_t i;
  for (i = 0; env[i].key != NULL; i++) {
    free((char *)env[i].key);
    free(env[i].step);
  }
  free(env[i].step);
  free(env);
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
