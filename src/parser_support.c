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
#include "parser_support.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// array of variables during parsing
static struct env_t *_vars = NULL;
// number of variables during parsing
static size_t _var_count = 0;

/** Type to represent a list of variables in a hash table */
struct var_list_t {
  size_t index; ///< Index of variable in _vars
  struct var_list_t *next; ///< Next element in list
};

// size of hash tables
#define TABLE_SIZE 4096
// table of keys/identifiers
static struct var_list_t *_keytab[TABLE_SIZE];
// table of variable values
static struct var_list_t *_valtab[TABLE_SIZE];

// append an element to a list of variables in a hash table
static struct var_list_t *var_list_append(struct var_list_t *list, size_t elem) {
  struct var_list_t *retval = (struct var_list_t *)malloc(sizeof(struct var_list_t));
  // die if allocation failed
  if (retval == NULL) {
    print_fatal("%s", strerror(errno));
  }
  retval->index = elem;
  retval->next = list;
  return retval;
}

// release memory for a list of variables in a hash table
static void var_list_free(struct var_list_t *list) {
  struct var_list_t *next;
  for (struct var_list_t *l = list; l != NULL; l = next) {
    next = l->next;
    free(l);
  }
}

// magic number for computing a string hash
#define HASH_STR_MAGIC 31

// compute a string hash
static uint32_t hash_str(const char *str) {
  int32_t hash = 0;
  while (*str != '\0') {
    hash = hash * HASH_STR_MAGIC + *str;
    str++;
  }
  return hash;
}

// add a variable to the keys/identifiers hash table
static void keytab_add(size_t index) {
  size_t hash = hash_str(_vars[index].key) % TABLE_SIZE;
  _keytab[hash] = var_list_append(_keytab[hash], index);
}

// find a variable in the keys/identifiers hash table
static struct env_t *keytab_find(const char *key) {
  size_t index = hash_str(key) % TABLE_SIZE;
  // iterate over list at hash table index
  for (struct var_list_t *l = _keytab[index]; l != NULL; l = l->next) {
    if (strcmp(key, _vars[l->index].key) == 0) {
      return &_vars[l->index];
    }
  }
  return NULL;
}

// release memory for the keys/identifiers hash table
static void keytab_free(void) {
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    var_list_free(_keytab[i]);
    _keytab[i] = NULL;
  }
}

// find a variable by its key/identifier
struct env_t *vars_find_key(const char *key) {
  return keytab_find(key);
}

// compute a constraint hash
static uint32_t hash_val(const struct constr_t *val) {
  return (intptr_t)val / sizeof(struct constr_t);
}

// add a variable to the variable values hash table
static void valtab_add(size_t index) {
  size_t hash = hash_val(_vars[index].val) % TABLE_SIZE;
  _valtab[hash] = var_list_append(_valtab[hash], index);
}

// find a variable in the variable values hash table
static struct env_t *valtab_find(const struct constr_t *val) {
  size_t index = hash_val(val) % TABLE_SIZE;
  // iterate over list at hash table index
  for (struct var_list_t *l = _valtab[index]; l != NULL; l = l->next) {
    if (val == _vars[l->index].val) {
      return &_vars[l->index];
    }
  }
  return NULL;
}

// release memory for the variable values hash table
static void valtab_free(void) {
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    var_list_free(_valtab[i]);
    _valtab[i] = NULL;
  }
}

// find a variable by its variable value
struct env_t *vars_find_val(const struct constr_t *val) {
  return valtab_find(val);
}

// return the number of variables during parsing
size_t var_count(void) {
  return _var_count;
}

// add a variable to the array of variables during parsing
void vars_add(const char *key, struct constr_t *val) {
  _var_count++;
  _vars = (struct env_t *)realloc(_vars, sizeof(struct env_t) * _var_count);

  // allocate memory for key/identifier
  char *k = (char *)malloc(strlen(key)+1);
  // die if allocation failed
  if (k == NULL) {
    print_fatal("%s", strerror(errno));
  }
  // copy key/identifier
  strcpy(k, key);

  // initialize variable
  _vars[_var_count-1] =
    (struct env_t){ .key = k,
                    .val = val,
                    .binds = NULL,
                    .clauses = { .length = 0, .elems = NULL },
                    .order = SIZE_MAX,
                    .prio = 0,
                    .level = SIZE_MAX };

  // add variable to key/identifier and variables values hash tables
  keytab_add(_var_count-1);
  valtab_add(_var_count-1);
}

// count the number of variables in an expression
int32_t vars_count(struct constr_t *constr) {
  if (IS_TYPE(TERM, constr)) {
    // count terminal if it is a variable
    if (!is_value(constr->constr.term.val)) {
      return 1;
    }
    return 0;
  }

  // recurse
  switch (constr->type->op) {
  case OP_EQ:
  case OP_LT:
  case OP_ADD:
  case OP_MUL:
  case OP_AND:
  case OP_OR:
    // count variables on right side
    return vars_count(constr->constr.expr.l) + vars_count(constr->constr.expr.r);
  case OP_NEG:
  case OP_NOT:
    // count variables on left side
    return vars_count(constr->constr.expr.l);
  default:
    // die if encountering an unknown operation
    print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
  }
  return 0;
}

// add variable weights to the variables in an expression
void vars_weighten(struct constr_t *constr, int32_t weight) {
  if (IS_TYPE(TERM, constr)) {
    // add weight to terminal
    if (!is_value(constr->constr.term.val)) {
      struct env_t *var = vars_find_val(constr);
      var->prio += weight;
    }
  } else {
    // recurse
    switch (constr->type->op) {
    case OP_EQ:
    case OP_LT:
    case OP_ADD:
    case OP_MUL:
    case OP_AND:
    case OP_OR:
      // weighten variables on right side
      vars_weighten(constr->constr.expr.r, weight);
      /* fall through */
    case OP_NEG:
    case OP_NOT:
      // weighten variables on left side
      vars_weighten(constr->constr.expr.l, weight);
      break;
    default:
      // die if encountering an unknown operation
      print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
    }
  }
}

// generate the variable environment
struct env_t *env_generate(void) {
  for (size_t i = 0; i < _var_count; i++) {
    struct val_t val = _vars[i].val->constr.term.val;
    // check if the variable is unbounded (after initial propagation/normalization)
    if (get_lo(val) == DOMAIN_MIN || get_hi(val) == DOMAIN_MAX) {
      print_fatal(ERROR_MSG_UNBOUNDED_VARIABLE, _vars[i].key);
    }
    // link back from terminal to environment
    _vars[i].val->constr.term.env = &_vars[i];
  }

  return _vars;
}

// release memory for the variable environment
void env_free(void) {
  keytab_free();
  valtab_free();

  for (size_t i = 0; i < _var_count; i++) {
    free((char *)_vars[i].key);
    free(_vars[i].clauses.elems);
  }
  free(_vars);

  _vars = NULL;
  _var_count = 0;
}

// add an element to a list of expressions
struct expr_list_t *expr_list_append(struct expr_list_t *list, struct constr_t *elem) {
  struct expr_list_t *retval = (struct expr_list_t *)malloc(sizeof(struct expr_list_t));
  // die if allocation failed
  if (retval == NULL) {
    print_fatal("%s", strerror(errno));
  }
  retval->expr = elem;
  retval->next = list;
  return retval;
}

// free the memory for a list of expressions
void expr_list_free(struct expr_list_t *list) {
  struct expr_list_t *next;
  for (struct expr_list_t *l = list; l != NULL; l = next) {
    next = l->next;
    free(l);
  }
}

// free memory for wide-and expression
static void expr_free_wand(struct constr_t *constr) {
  for (size_t i = 0; i < constr->constr.wand.length; i++) {
    expr_free(constr->constr.wand.elems[i].constr);
  }
  free(constr->constr.wand.elems);
}

// free memory for ordinary expressions
static void expr_free_constr(struct constr_t *constr) {
  // recurse
  switch (constr->type->op) {
  case OP_EQ:
  case OP_LT:
  case OP_ADD:
  case OP_MUL:
  case OP_AND:
  case OP_OR:
    // weighten variables on right side
    expr_free(constr->constr.expr.r);
    /* fall through */
  case OP_NEG:
  case OP_NOT:
    // weighten variables on left side
    expr_free(constr->constr.expr.l);
    break;
  default:
    // die if encountering an unknown operation
    print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
  }
}

// free the memory allocated for wide-and nodes in an expression
void expr_free(struct constr_t *constr) {
  if (IS_TYPE(TERM, constr)) {
    /* nothing to do */
  } else if (IS_TYPE(WAND, constr)) {
    expr_free_wand(constr);
  } else {
    expr_free_constr(constr);
  }
}

// initialize clause list for terminal
static void clauses_init_term(struct constr_t *constr, struct wand_expr_t *clause) {
  // add clause to terminal
  if (!is_value(constr->constr.term.val) && clause != NULL) {
    struct env_t *e = constr->constr.term.env;
    if (!clause_list_contains(&e->clauses, clause)) {
      clause_list_append(&e->clauses, clause);
    }
  }
}

// initialize clause list for wide-and expression
static void clauses_init_wand(struct constr_t *constr, struct wand_expr_t *clause) {
  // recurse to wide-and sub-expressions
  for (size_t i = 0, l = constr->constr.wand.length; i < l; i++) {
    struct wand_expr_t *c = clause;
    // set clause to sub-expression, unless it is already set or the
    // sub-expression itself is a wide-and expression
    if (clause == NULL && !IS_TYPE(WAND, constr->constr.wand.elems[i].constr)) {
      c = &constr->constr.wand.elems[i];
    }
    clauses_init(constr->constr.wand.elems[i].constr, c);
  }
}

// initialize clause list for ordinary expressions
static void clauses_init_expr(struct constr_t *constr, struct wand_expr_t *clause) {
  // recurse
  switch (constr->type->op) {
  case OP_EQ:
  case OP_LT:
  case OP_ADD:
  case OP_MUL:
  case OP_AND:
  case OP_OR:
    // initialize clauses on right side
    clauses_init(constr->constr.expr.r, clause);
    /* fall through */
  case OP_NEG:
  case OP_NOT:
    // initialize clauses on left side
    clauses_init(constr->constr.expr.l, clause);
    break;
  default:
    // die if encountering an unknown operation
    print_fatal(ERROR_MSG_INVALID_OPERATION, constr->type->op);
  }
}

// initialize clause lists for variables in an expression
void clauses_init(struct constr_t *constr, struct wand_expr_t *clause) {
  if (IS_TYPE(TERM, constr)) {
    clauses_init_term(constr, clause);
  } else if (IS_TYPE(WAND, constr)) {
    clauses_init_wand(constr, clause);
  } else {
    clauses_init_expr(constr, clause);
  }
}
