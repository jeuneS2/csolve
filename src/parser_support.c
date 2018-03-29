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

struct var_t *vars = NULL;
size_t var_count = 0;

struct var_t *vars_find_key(const char *key) {
  for (size_t i = 0; i < var_count; i++) {
    if (strcmp(vars[i].var.key, key) == 0) {
      return &vars[i];
    }
  }
  return NULL;
}
struct var_t *vars_find_val(const struct val_t *val) {
  for (size_t i = 0; i < var_count; i++) {
    if (vars[i].var.val == val) {
      return &vars[i];
    }
  }
  return NULL;
}

void vars_add(const char *key, struct val_t *val) {
  var_count++;
  vars = realloc(vars, sizeof(struct var_t) * var_count);
  vars[var_count-1].var.key = malloc(strlen(key)+1);
  strcpy((char *)vars[var_count-1].var.key, key);
  vars[var_count-1].var.val = val;
  vars[var_count-1].weight = 0;
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
      fprintf(stderr, ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  default:
    fprintf(stderr, ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
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
      fprintf(stderr, ERROR_MSG_INVALID_OPERATION, constr->constr.expr.op);
    }
    break;
  default:
    fprintf(stderr, ERROR_MSG_INVALID_CONSTRAINT_TYPE, constr->type);
  }
}

int vars_compare(const void *a, const void *b) {
  const struct var_t *x = (const struct var_t *)a;
  const struct var_t *y = (const struct var_t *)b;
  return y->weight - x->weight;
}

void vars_sort(void) {
  qsort(vars, var_count, sizeof(struct var_t), vars_compare);
}

void vars_print(FILE *file) {
  for (size_t i = 0; i < var_count; i++) {
    fprintf(file, "%s: %d", vars[i].var.key, vars[i].weight);
    print_val(file, *vars[i].var.val);
    fprintf(file, "\n");
  }
}

struct env_t *generate_env() {
  struct env_t *env = malloc(sizeof(struct env_t) * (var_count+1));
  for (size_t i = 0; i < var_count; i++) {
    env[i] = vars[i].var;
  }
  env[var_count].key = NULL;
  env[var_count].val = NULL;
  return env;
}
