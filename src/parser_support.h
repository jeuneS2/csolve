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

#ifndef PARSER_SUPPORT_H
#define PARSER_SUPPORT_H

/** Type to represent a variable during parsing */
struct var_t {
  struct env_t var; ///< Variable (key/value pair)
  uint32_t weight; ///< Weight of variable
};

/** Weight for variables in an "equal" constraint */
#define WEIGHT_EQUAL     1000
/** Weight for variables in a comparison constraint (less-than, greater-than, ...) */
#define WEIGHT_COMPARE   100
/** Weight for variables in a "not-equal" constraint */
#define WEIGHT_NOT_EQUAL 10

/** Find variable with the given key */
struct var_t *vars_find_key(const char *key);
/** Add a new variable */
void vars_add(const char *key, struct val_t *val);

/** Count the number of variables in a constarint */
int32_t vars_count(struct constr_t *constr);
/** Weighten the variables in a constraint */
void vars_weighten(struct constr_t *constr, int32_t weight);

/** Sort variables according to their weight */
void vars_sort(void);

/** Generate variable environment */
struct env_t *env_generate(void);
/** Deallocate memory occipied by variable environment */
void env_free(struct env_t *);

/** Type to represent a list of expressions during parsing */
struct expr_list_t {
  struct constr_t *expr; ///< Expression
  struct expr_list_t *next; ///< Next element of list
};

/** Add an element to a list of expressions */
struct expr_list_t *expr_list_append(struct expr_list_t *list, struct constr_t *elem);
/** Deallocate memory occupied by list of expressions */
void expr_list_free(struct expr_list_t *list);

#endif
