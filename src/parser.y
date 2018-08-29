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

int yylex(void);
void yyerror(const char *);

%}

%union {
  const char *strval;
  domain_t intval;
  struct constr_t *expr;
  struct expr_list_t *expr_list;
}

%locations
%define parse.error verbose
%define parse.lac full

%token ANY ALL MIN MAX NEQ LEQ GEQ ALL_DIFFERENT
%token <intval> NUM
%token <strval> IDENT

%type <expr> PrimaryExpr UnaryExpr MultExpr AddExpr RelatExpr EqualExpr AndExpr OrExpr Expr
%type <expr> Objective Constraint Constraints
%type <expr_list> ExprList

%start Input

%%

Input : Constraints
      {
        prop_result_t prop = propagate($1);
        struct constr_t *norm = $1;

        if (prop != PROP_ERROR) {
          struct constr_t *prev;
          do {
            prev = norm;
            norm = normalize(norm);
            prop = propagate(norm);
          } while (norm != prev && prop != PROP_ERROR);
        }

        stats_init();

        if (prop != PROP_ERROR) {
          size_t size = var_count();
          struct env_t *env = env_generate();

          clauses_init(norm, NULL);
          strategy_var_order_init(size, env);

          solve(size, env, norm);

          env_free();
        }
      }

Constraints : Constraints Constraint
            { $$->constr.wand.length = $1->constr.wand.length + 1;
              const size_t size = $$->constr.wand.length * sizeof(struct wand_expr_t);
              $$->constr.wand.elems = realloc($1->constr.wand.elems, size);
              $$->constr.wand.elems[$$->constr.wand.length-1].constr = $2;
              $$->constr.wand.elems[$$->constr.wand.length-1].prop_tag = 0;
            }
            | Objective
            { $$ = alloc(sizeof(struct constr_t));
              *$$ = CONSTRAINT_WAND(1, malloc(sizeof(struct wand_expr_t)));
              $$->constr.wand.elems[0].constr = $1;
              $$->constr.wand.elems[0].prop_tag = 0;
            }
;

Objective : ANY ';'
          { objective_init(OBJ_ANY, &shared()->objective_best);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
            *$$->constr.term = VALUE(1);
          }
          | ALL ';'
          { objective_init(OBJ_ALL, &shared()->objective_best);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
            *$$->constr.term = VALUE(1);
          }
          | MIN Expr ';'
          { objective_init(OBJ_MIN, &shared()->objective_best);
            struct constr_t *v = alloc(sizeof(struct constr_t));
            *v = CONSTRAINT_TERM(objective_val());
            vars_add("<obj>", objective_val());
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(EQ, $2, v);
          }
          | MAX Expr ';'
          { objective_init(OBJ_MAX, &shared()->objective_best);
            struct constr_t *v = alloc(sizeof(struct constr_t));
            *v = CONSTRAINT_TERM(objective_val());
            vars_add("<obj>", objective_val());
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(EQ, v, $2);
          }
;

Constraint: Expr ';';

PrimaryExpr : NUM
            { $$ = alloc(sizeof(struct constr_t));
              *$$ = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
              *$$->constr.term = VALUE($1);
            }
            | IDENT
            { $$ = alloc(sizeof(struct constr_t));
              struct env_t *var = vars_find_key($1);
              if (var != NULL) {
                *$$ = CONSTRAINT_TERM(var->val);
              } else {
                *$$ = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
                *$$->constr.term = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
                vars_add($1, $$->constr.term);
              }
            }
            | '(' Expr ')'
            { $$ = $2;
            }
;

UnaryExpr : PrimaryExpr
          | '-' PrimaryExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(NEG, $2, NULL);
          }
          | '!' PrimaryExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(NOT, $2, NULL);
          }
          | ALL_DIFFERENT '(' ExprList ')'
          {
            // create wide-and node
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_WAND(0, NULL);
            // add != constraints for all pairs
            for (struct expr_list_t *l = $3; l != NULL; l = l->next) {
              for (struct expr_list_t *k = l->next; k != NULL; k = k->next) {
                struct constr_t *a = alloc(sizeof(struct constr_t));
                *a = CONSTRAINT_EXPR(LT, l->expr, k->expr);
                struct constr_t *b = alloc(sizeof(struct constr_t));
                *b = CONSTRAINT_EXPR(LT, k->expr, l->expr);
                struct constr_t *c = alloc(sizeof(struct constr_t));
                *c = CONSTRAINT_EXPR(OR, a, b);

                $$->constr.wand.length++;
                const size_t size = $$->constr.wand.length * sizeof(struct wand_expr_t);
                $$->constr.wand.elems = realloc($$->constr.wand.elems, size);
                $$->constr.wand.elems[$$->constr.wand.length-1].constr = c;
                $$->constr.wand.elems[$$->constr.wand.length-1].prop_tag = 0;
              }
            }
            expr_list_free($3);
          }
;

ExprList : Expr
         { $$ = expr_list_append(NULL, $1);
         }
         | ExprList ',' Expr
         { $$ = expr_list_append($1, $3);
         }
;

MultExpr : UnaryExpr
         | MultExpr '*' UnaryExpr
         { $$ = alloc(sizeof(struct constr_t));
           *$$ = CONSTRAINT_EXPR(MUL, $1, $3);
         }
;

AddExpr : MultExpr
        | AddExpr '+' MultExpr
        { $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(ADD, $1, $3);
        }
        | AddExpr '-' MultExpr
        { struct constr_t *c = alloc(sizeof(struct constr_t));
          *c = CONSTRAINT_EXPR(NEG, $3, NULL);
          $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(ADD, $1, c);
        }
;

RelatExpr : AddExpr
          | RelatExpr '<' AddExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(LT, $1, $3);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
            }
          }
          | RelatExpr '>' AddExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(LT, $3, $1);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
            }
          }
          | RelatExpr LEQ AddExpr
          { struct constr_t *v = alloc(sizeof(struct constr_t));
            *v = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
            *v->constr.term = VALUE(1);
            struct constr_t *c = alloc(sizeof(struct constr_t));
            *c = CONSTRAINT_EXPR(ADD, $3, v);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(LT, $1, c);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
            }
          }
          | RelatExpr GEQ AddExpr
          { struct constr_t *v = alloc(sizeof(struct constr_t));
            *v = CONSTRAINT_TERM(alloc(sizeof(struct val_t)));
            *v->constr.term = VALUE(1);
            struct constr_t *c = alloc(sizeof(struct constr_t));
            *c = CONSTRAINT_EXPR(ADD, $1, v);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(LT, $3, c);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_COMPARE / max(1, vars_count($$)));
            }
          }
;

EqualExpr : RelatExpr
          | EqualExpr '=' RelatExpr
          { $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(EQ, $1, $3);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_EQUAL / max(1, vars_count($$)));
            }
          }
          | EqualExpr NEQ RelatExpr
          { struct constr_t *a = alloc(sizeof(struct constr_t));
            *a = CONSTRAINT_EXPR(LT, $1, $3);
            struct constr_t *b = alloc(sizeof(struct constr_t));
            *b = CONSTRAINT_EXPR(LT, $3, $1);
            $$ = alloc(sizeof(struct constr_t));
            *$$ = CONSTRAINT_EXPR(OR, a, b);
            if (strategy_compute_weights()) {
              vars_weighten($$, WEIGHT_NOT_EQUAL / max(1, vars_count($$)));
            }
          }
;

AndExpr : EqualExpr
        | AndExpr '&' EqualExpr
        { $$ = alloc(sizeof(struct constr_t));
          *$$ = CONSTRAINT_EXPR(AND, $1, $3);
        }
;

OrExpr : AndExpr
       | OrExpr '|' AndExpr
       { $$ = alloc(sizeof(struct constr_t));
         *$$ = CONSTRAINT_EXPR(OR, $1, $3);
       }
;

Expr : OrExpr
;

%%

void yyerror(const char *message) {
  print_error(ERROR_MSG_PARSER_ERROR, message, yylloc.first_line);
}
