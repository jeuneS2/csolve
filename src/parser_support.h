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

#ifndef PARSER_SUPPORT_H
#define PARSER_SUPPORT_H

/** Weight for variables in an "equal" constraint */
#define WEIGHT_EQUAL     1000
/** Weight for variables in a comparison constraint (less-than, greater-than, ...) */
#define WEIGHT_COMPARE   100
/** Weight for variables in a "not-equal" constraint */
#define WEIGHT_NOT_EQUAL 10

/** Total number of variables */
size_t var_count(void);

/** Find variable with the given key */
struct env_t *vars_find_key(const char *key);
/** Add a new variable */
void vars_add(const char *key, struct constr_t *val);

/** Count the number of variables in a constarint */
int32_t vars_count(struct constr_t *constr);
/** Weighten the variables in a constraint */
void vars_weighten(struct constr_t *constr, int32_t weight);

/** Generate variable environment */
struct env_t *env_generate(void);
/** Deallocate memory occupied by variable environment */
void env_free(void);

/** Type to represent a list of expressions during parsing */
struct expr_list_t {
  struct constr_t *expr; ///< Expression
  struct expr_list_t *next; ///< Next element of list
};

/** Add an element to a list of expressions */
struct expr_list_t *expr_list_append(struct expr_list_t *list, struct constr_t *elem);
/** Deallocate memory occupied by list of expressions */
void expr_list_free(struct expr_list_t *list);

/** Free the memory allocated for wide-and nodes in an expression */
void expr_free(struct constr_t *constr);

/** Initialize clause lists */
void clauses_init(struct constr_t *constr, struct wand_expr_t *clause);

#endif
