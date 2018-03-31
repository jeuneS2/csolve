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

#ifndef CSOLVE_H
#define CSOLVE_H

#include <stdint.h>
#include <stdbool.h>

/** The type representing the value domain */
typedef int32_t domain_t;

/** Minimum value representable in domain */
#define DOMAIN_MIN INT32_MIN
/** Maximum value representable in domain */
#define DOMAIN_MAX INT32_MAX

/** Number of bits of values in domain */
#define DOMAIN_BITS 32
/** Unsigned type corresponding to domain type, used for arithmetic */
typedef uint32_t udomain_t;
/** Double-width type corresponding to domain type, used for arithmetic */
typedef int64_t ddomain_t;

/** Type of a value */
enum val_type_t {
  VAL_INTERVAL, ///< Value is an interval
  VAL_VALUE     ///< Value is a single value
};

/** Type representing a value, either a single value or an interval */
struct val_t {
  enum val_type_t type; ///< Type of this value
  /** Union to hold either single value or interval */
  union value_union_t {
    /** Interval type */
    struct interval_t {
      domain_t lo; ///< Lower bound of interval
      domain_t hi; ///< Upper bound of interval
    } ivl; ///< Interval
    domain_t val; ///< Single value
  } value; ///< Value
};

/** Checks whether value contains a single value */
static inline bool is_value(struct val_t v) {
  return v.type == VAL_VALUE;
}
/** Get the lower bound of an interval or the single value */
static inline domain_t get_lo(struct val_t v) {
  return is_value(v) ? v.value.val : v.value.ivl.lo;
}
/** Get the upper bound of an interval or the single value */
static inline domain_t get_hi(struct val_t v) {
  return is_value(v) ? v.value.val : v.value.ivl.hi;
}
/** Checks whether value represents a boolean "true" */
static inline bool is_true(struct val_t v) {
  return get_lo(v) > 0 || get_hi(v) < 0;
}
/** Checks whether value represents a boolean "false" */
static inline bool is_false(struct val_t v) {
  return is_value(v) && v.value.val == 0;
}

#define VALUE(V)                                    \
  ((struct val_t){                                  \
    .type = VAL_VALUE,                              \
      .value = { .val = V } })
#define INTERVAL(L,H)                               \
  ((struct val_t){                                  \
    .type = VAL_INTERVAL,                           \
      .value = { .ivl = { .lo = L, .hi = H } } })

/** Variable environment entry */
struct env_t {
  const char *key; ///< Key (identifier) of variable
  struct val_t *val; ///< Value of variable
};


/** Binding entry */
struct binding_t {
  struct val_t *loc; ///< Location of bound value
  struct val_t val; ///< Original value of bound value (for unbinding)
};


/** Constraint node types */
enum constr_type_t {
  CONSTR_TERM, ///< Terminal node
  CONSTR_EXPR  ///< Expression node
};

/** Supported operators */
enum operator_t {
  OP_EQ  = '=', ///< Equality
  OP_LT  = '<', ///< Less-than
  OP_NEG = '-', ///< Negation
  OP_ADD = '+', ///< Addition
  OP_MUL = '*', ///< Multiplication
  OP_NOT = '!', ///< Logical not
  OP_AND = '&', ///< Logical and
  OP_OR  = '|', ///< Logical or
};

/** The type to use for the evaliation cache tag */
typedef int64_t cache_tag_t;

/** Type representing a constraint */
struct constr_t {
  enum constr_type_t type; ///< Type of constraint node
  /** Union to hold either terminal node or expression node */
  union constr_union_t {
    struct val_t *term; ///< Value of terminal node
    /** Expression node type */
    struct expr_t {
      enum operator_t op; ///< Operator of expression
      struct constr_t *l; ///< Left child of expression
      struct constr_t *r; ///< Right child of expression, NULL for unary operators
    } expr; ///< Expression node
  } constr; ///< Constraint
  /** Cache entry for evaluation results */
  struct eval_cache_t {
    cache_tag_t tag; ///< Cache tag
    struct val_t val; ///< Cached value
  } eval_cache; ///< Evaluation result cache
};

/** Checks whether constraint is a terminal node */
static inline bool is_term(struct constr_t *c) {
  return c->type == CONSTR_TERM;
}
/** Checks whether constraint is a constant value */
static inline bool is_const(struct constr_t *c) {
  return is_term(c) && is_value(*c->constr.term);
}

#define CONSTRAINT_EXPR(O, L, R)                         \
  ((struct constr_t){                                    \
    .type = CONSTR_EXPR,                                 \
      .constr = { .expr = { .op = O, .l = L, .r = R } }, \
      .eval_cache = { .tag = 0, .val = VALUE(0) } })


/** Types of objective functions */
enum objective_t {
  OBJ_ANY, ///< Find any solution
  OBJ_ALL, ///< Find all solutions
  OBJ_MIN, ///< Find solution that minimizes objective function
  OBJ_MAX  ///< Find solution that maximizes objective function
};

/** Negate a value */
const domain_t neg(const domain_t a);
/** Add two values */
const domain_t add(const domain_t a, const domain_t b);
/** Multiply two values */
const domain_t mul(const domain_t a, const domain_t b);
/** Return minimum of two values */
const domain_t min(const domain_t a, const domain_t b);
/** Return maximum of two values */
const domain_t max(const domain_t a, const domain_t b);

/** Allocate memory on the allocation stack */
void *alloc(size_t size);
/** Deallocate memory on the allocation stack */
void dealloc(void *elem);

/** Bind a variable (at location loc) to a specific value */
size_t bind(struct val_t *loc, const struct val_t val);
/** Undo variable binds down to a given depth */
void unbind(size_t depth);

/** Evaluate a constraint */
const struct val_t eval(const struct constr_t *constr);
/** Normalize a constraint */
struct constr_t *normalize(struct constr_t *constr);
/** Propagate a result value to the terminal nodes of the constraint */
struct constr_t *propagate(struct constr_t *constr, struct val_t val);

/** Update children of constraint, possibly allocating a new node */
struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r);
/** Update child of constraint with unary operator, possibly allocating a new node */
struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l);

/** Invalidate the eval cache */
void eval_cache_invalidate(void);

/** Initialize objective function type */
void objective_init(enum objective_t o);
/** Get objective function type */
enum objective_t objective(void);
/** Check whether the objective value can be possibly better */
bool objective_better(struct constr_t *obj);
/** Update the objective value */
void objective_update(struct val_t obj);
/** Get the current best objective value */
domain_t objective_best(void);
/** Optimize objective function */
struct constr_t *objective_optimize(struct constr_t *obj);

/** Find solutions */
void solve(struct env_t *env, struct constr_t *obj, struct constr_t *constr, size_t depth);

/** Print value */
void print_val(FILE *file, const struct val_t val);
/** Print constraint */
void print_constr(FILE *file, const struct constr_t *constr);
/** Print variable environment/bindings */
void print_env(FILE *file, struct env_t *env);
/** Print statistics */
void print_stats(FILE *file);
/** Print a solution */
void print_solution(FILE *file, struct env_t *env);
/** Print an error message */
void print_error(const char *fmt, ...);

/** Get the name of the program */
const char *main_name(void);

/** Error message for errors in the lexer */
#define ERROR_MSG_LEXER_ERROR               "invalid input `%c' in line %u"
/** Error message for errors in the parser */
#define ERROR_MSG_PARSER_ERROR              "%s in line %u"
/** Error message when running out of memory */
#define ERROR_MSG_OUT_OF_MEMORY             "out of memory"
/** Error message when errors when deallocating memory */
#define ERROR_MSG_WRONG_DEALLOC             "wrong deallocation"
/** Error message when exceeding the maximum number of binds */
#define ERROR_MSG_TOO_MANY_BINDS            "exceeded maximum number of binds"
/** Error message when trying to bind NULL */
#define ERROR_MSG_NULL_BIND                 "cannot bind NULL"
/** Error message when encountering an invalid constraint type */
#define ERROR_MSG_INVALID_CONSTRAINT_TYPE   "invalid constraint type: %02x"
/** Error message when encountering an invalid operation */
#define ERROR_MSG_INVALID_OPERATION         "invalid operation: %02x"
/** Error message when encountering an invalid variable type */
#define ERROR_MSG_INVALID_VARIABLE_TYPE     "invalid variable type: %02x"
/** Error message when encountering an invalid objective function type */
#define ERROR_MSG_INVALID_OBJ_FUNC_TYPE     "invalid objective function type: %02x"
/** Error message when trying to update the best value with an interval */
#define ERROR_MSG_UPDATE_BEST_WITH_INTERVAL "trying to update best value with interval"

#endif
