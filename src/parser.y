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
#include "parser_support.h"

extern FILE *yyin;
int yylex(void);
void yyerror(const char *);

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
        vars_sort();

        struct constr_t *norm = normalize($2);
        struct constr_t *prop = propagate(norm, VALUE(1));

        if (prop != NULL) {
          do {
            norm = normalize(prop);
            prop = propagate(norm, VALUE(1));
          } while (norm != prop && prop != NULL);
        }

        if (prop != NULL) {
          struct env_t *env = generate_env();
          struct constr_t *obj = objective_optimize($1);
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
              struct var_t *var = vars_find_key($1);
              if (var != NULL) {
                $$->constr.term = var->var.val;
              } else {
                $$->constr.term = alloc(sizeof(struct val_t));
                *$$->constr.term = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
                vars_add($1, $$->constr.term);
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
            vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
          }
          | RelatExpr '>' AddExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_LT, $3, $1);
            vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
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
            vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
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
            vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
          }
;

EqualExpr : RelatExpr
          | EqualExpr '=' RelatExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_EQ, $1, $3);
            vars_weighten($$, WEIGHT_EQUAL / max(1, vars_count($$)));
          }
          | EqualExpr NEQ RelatExpr
          { struct constr_t *a = alloc(sizeof(struct constr_t));
            *a = CONSTRAINT_EXPR(OP_LT, $1, $3);
            struct constr_t *b = alloc(sizeof(struct constr_t));
            *b = CONSTRAINT_EXPR(OP_LT, $3, $1);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OP_OR, a, b);
            vars_weighten($$, WEIGHT_NOT_EQUAL / max(1, vars_count($$)));
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

void yyerror(const char *message) {
  fprintf(stderr, ERROR_MSG_PARSER_ERROR, message, yylloc.first_line);
}

int main(int argc, char **argv) {
  yyin = stdin;
  return yyparse();
}
