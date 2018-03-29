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

%{
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "csolve.h"

extern FILE *yyin;
int yylex(void);
void yyerror(const char *);

struct var_t {
  struct env_t var;
  uint32_t weight;
};
struct var_t *vars = NULL;
size_t var_count = 0;

struct var_t *find_key_in_vars(const char *key);
struct var_t *find_val_in_vars(const struct val_t *val);
void add_to_vars(const char *key, struct val_t *val);
void print_vars(FILE *file);

int32_t count_vars(struct constr_t *constr);
void weighten(struct constr_t *constr, int32_t weight);

int compare_vars(const void *a, const void *b);

struct env_t *generate_env(void);

%}

%union {
  const char *strval;
  domain_t intval;
  struct constr_t *constr;
}

%locations
%define parse.error verbose
%define parse.lac full

%token ANY ALL MIN MAX NEQ LEQ GEQ
%token <intval> NUM
%token <strval> IDENT

%type <constr> PrimaryExpr UnaryExpr MultExpr AddExpr RelatExpr EqualExpr AndExpr OrExpr Expr
%type <constr> Objective Constraint Constraints

%start Input

%%

Input : Objective Constraints
      {
        qsort(vars, var_count, sizeof(struct var_t), compare_vars);
        struct env_t *env = generate_env();

        struct constr_t *norm = normalize(env, $2);
        struct constr_t *prop = propagate(env, norm, VALUE(1));

        if (prop != NULL) {
          do {
            norm = normalize(env, prop);
            prop = propagate(env, norm, VALUE(1));
          } while (norm != prop && prop != NULL);
        }

        if (prop != NULL) {
          struct constr_t *obj = objective_optimize(env, $1);
          solve(env, obj, prop, 0);
        }

        print_stats(stdout);
      }

Objective : ANY ';'
          { objective_init(OBJ_ANY);
            $$ = alloc(sizeof(struct constr_t));
            $$->type = CONSTR_TERM;
            $$->constr.term = alloc(sizeof(struct val_t));
            *$$->constr.term = VALUE(0);
            $$->eval_cache.tag = 0;
            $$->eval_cache.val = VALUE(0);
          }
          | ALL ';'
          { objective_init(OBJ_ALL);
            $$ = alloc(sizeof(struct constr_t));
            $$->type = CONSTR_TERM;
            $$->constr.term = alloc(sizeof(struct val_t));
            *$$->constr.term = VALUE(0);
            $$->eval_cache.tag = 0;
            $$->eval_cache.val = VALUE(0);
          }
          | MIN Expr ';'
          { objective_init(OBJ_MIN);
            $$ = $2;
          }
          | MAX Expr ';'
          { objective_init(OBJ_MAX);
            $$ = $2;
          }
;

Constraints : Constraints Constraint
            { $$ = alloc(sizeof(struct constr_t));
              *$$ = CONSTRAINT_EXPR(OP_AND, $1, $2);
            }
            | Constraint
            { $$ = $1;
            }
;

Constraint: Expr ';';

PrimaryExpr : NUM
            { $$ = alloc(sizeof(struct constr_t));
              $$->type = CONSTR_TERM;
              $$->constr.term = alloc(sizeof(struct val_t));
              *$$->constr.term = VALUE($1);
              $$->eval_cache.tag = 0;
              $$->eval_cache.val = VALUE($1);
            }
            | IDENT
            { $$ = alloc(sizeof(struct constr_t));
              $$->type = CONSTR_TERM;
              struct var_t *var = find_key_in_vars($1);
              if (var != NULL) {
                $$->constr.term = var->var.val;
              } else {
                $$->constr.term = alloc(sizeof(struct val_t));
                *$$->constr.term = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
                add_to_vars($1, $$->constr.term);
              }
              $$->eval_cache.tag = 0;
              $$->eval_cache.val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
            }
            | '(' Expr ')'
            { $$ = $2;
            }
;

UnaryExpr : PrimaryExpr
          | '-' PrimaryExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_NEG, $2, NULL);
          }
          | '!' PrimaryExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_NOT, $2, NULL);
          }


MultExpr : UnaryExpr
         | MultExpr '*' UnaryExpr
         { $$ = alloc(sizeof(struct constr_t));
           *$$ = CONSTRAINT_EXPR(OP_MUL, $1, $3);
         }
;

AddExpr : MultExpr
        | AddExpr '+' MultExpr
        { $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(OP_ADD, $1, $3);
        }
        | AddExpr '-' MultExpr
        { struct constr_t *c = alloc(sizeof(struct constr_t));
          *c = CONSTRAINT_EXPR(OP_NEG, $3, NULL);
          $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(OP_ADD, $1, c);
        }
;

RelatExpr : AddExpr
          | RelatExpr '<' AddExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_LT, $1, $3);
            weighten($$, 100 / max(1, count_vars($$)));
          }
          | RelatExpr '>' AddExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_LT, $3, $1);
            weighten($$, 100 / max(1, count_vars($$)));
          }
          | RelatExpr LEQ AddExpr
          { struct constr_t *v = alloc(sizeof(struct constr_t));
            v->type = CONSTR_TERM;
            v->constr.term = alloc(sizeof(struct val_t));
            *v->constr.term = VALUE(1);
            v->eval_cache.tag = 0;
            v->eval_cache.val = VALUE(1);
            struct constr_t *c = alloc(sizeof(struct constr_t));
            *c = CONSTRAINT_EXPR(OP_ADD, $3, v);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_LT, $1, c);
            weighten($$, 100 / max(1, count_vars($$)));
          }
          | RelatExpr GEQ AddExpr
          { struct constr_t *v = alloc(sizeof(struct constr_t));
            v->type = CONSTR_TERM;
            v->constr.term = alloc(sizeof(struct val_t));
            *v->constr.term = VALUE(1);
            v->eval_cache.tag = 0;
            v->eval_cache.val = VALUE(1);
            struct constr_t *c = alloc(sizeof(struct constr_t));
            *c = CONSTRAINT_EXPR(OP_ADD, $1, v);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_LT, $3, c);
            weighten($$, 100 / max(1, count_vars($$)));
          }
;

EqualExpr : RelatExpr
          | EqualExpr '=' RelatExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_EQ, $1, $3);
            weighten($$, 1000 / max(1, count_vars($$)));
          }
          | EqualExpr NEQ RelatExpr
          { struct constr_t *a = alloc(sizeof(struct constr_t));
            *a = CONSTRAINT_EXPR(OP_LT, $1, $3);
            struct constr_t *b = alloc(sizeof(struct constr_t));
            *b = CONSTRAINT_EXPR(OP_LT, $3, $1);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_OR, a, b);
            weighten($$, 10 / max(1, count_vars($$)));
          }
;

AndExpr : EqualExpr
        | AndExpr '&' EqualExpr
        { $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(OP_AND, $1, $3);
        }
;

OrExpr : AndExpr
       | OrExpr '|' AndExpr
       { $$ = alloc(sizeof(struct constr_t));
         *$$ = CONSTRAINT_EXPR(OP_OR, $1, $3);
       }
;

Expr : OrExpr
;

%%

struct var_t *find_key_in_vars(const char *key) {
  for (size_t i = 0; i < var_count; i++) {
    if (strcmp(vars[i].var.key, key) == 0) {
      return &vars[i];
    }
  }
  return NULL;
}
struct var_t *find_val_in_vars(const struct val_t *val) {
  for (size_t i = 0; i < var_count; i++) {
    if (vars[i].var.val == val) {
      return &vars[i];
    }
  }
  return NULL;
}

void add_to_vars(const char *key, struct val_t *val) {
  var_count++;
  vars = realloc(vars, sizeof(struct var_t) * var_count);
  vars[var_count-1].var.key = malloc(strlen(key)+1);
  strcpy((char *)vars[var_count-1].var.key, key);
  vars[var_count-1].var.val = val;
  vars[var_count-1].weight = 0;
}

int32_t count_vars(struct constr_t *constr) {
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
      return count_vars(constr->constr.expr.l) + count_vars(constr->constr.expr.r);
    case OP_NEG:
    case OP_NOT:
      return count_vars(constr->constr.expr.l);
    default:
      fprintf(stderr, "ERROR: invalid operation: %02x\n", constr->constr.expr.op);
    }
    break;
  default:
    fprintf(stderr, "ERROR: invalid constraint type: %02x\n", constr->type);
  }
  return 0;
}

void weighten(struct constr_t *constr, int32_t weight) {
  switch (constr->type) {
  case CONSTR_TERM:
    if (!is_value(*constr->constr.term)) {
      struct var_t *var = find_val_in_vars(constr->constr.term);
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
      weighten(constr->constr.expr.r, weight);
      /* fall through */
    case OP_NEG:
    case OP_NOT:
      weighten(constr->constr.expr.l, weight);
      break;
    default:
      fprintf(stderr, "ERROR: invalid operation: %02x\n", constr->constr.expr.op);
    }
    break;
  default:
    fprintf(stderr, "ERROR: invalid constraint type: %02x\n", constr->type);
  }
}

int compare_vars(const void *a, const void *b) {
  const struct var_t *x = (const struct var_t *)a;
  const struct var_t *y = (const struct var_t *)b;
  return y->weight - x->weight;
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

void print_vars(FILE *file) {
  for (size_t i = 0; i < var_count; i++) {
    fprintf(file, "%s: %d", vars[i].var.key, vars[i].weight);
    print_val(file, *vars[i].var.val);
    fprintf(file, "\n");
  }
}

void yyerror(const char *message) {
  fprintf(stderr, "ERROR: %s in line %u\n", message, yylloc.first_line);
}

int main(int argc, char **argv) {
  yyin = stdin;
  return yyparse();
}
