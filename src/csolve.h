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
#include <semaphore.h>

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

/** Type representing a value (an interval) */
struct val_t {
  domain_t lo; ///< Lower bound of interval
  domain_t hi; ///< Upper bound of interval
};

/** Get the lower bound of an interval or the single value */
static inline domain_t get_lo(struct val_t v) {
  return v.lo;
}
/** Get the upper bound of an interval or the single value */
static inline domain_t get_hi(struct val_t v) {
  return v.hi;
}
/** Checks whether value contains a single value */
static inline bool is_value(struct val_t v) {
  return get_lo(v) == get_hi(v);
}
/** Checks whether value represents a boolean "true" */
static inline bool is_true(struct val_t v) {
  return get_lo(v) > 0 || get_hi(v) < 0;
}
/** Checks whether value represents a boolean "false" */
static inline bool is_false(struct val_t v) {
  return is_value(v) && get_lo(v) == 0;
}

#define INTERVAL(L,H) ((struct val_t){ .lo = (L), .hi = (H) })
#define VALUE(V) INTERVAL(V,V)

/** Binding entry */
struct binding_t {
  struct val_t *loc; ///< Location of bound value
  struct val_t val; ///< Original value of bound value (for unbinding)
};

/** Constraint node types */
enum constr_type_t {
  CONSTR_TERM, ///< Terminal node
  CONSTR_EXPR, ///< Expression node
  CONSTR_WAND  ///< Wide-and node
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

/** Type for tagging cache entries */
typedef uint64_t cache_tag_t;

/** Type for a wide-and element, including a cache tag */
struct wand_expr_t {
  struct constr_t *constr; ///< The constraint
  cache_tag_t cache_tag; ///< The cache tag
};

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
    /** Wide-and node type */
    struct wand_t {
      size_t length; ///< Number of sub-expressions
      struct wand_expr_t *elems; ///< Sub-expressions
    } wand; ///< Wide-and node
  } constr; ///< Constraint
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
      .constr = { .expr = { .op = O, .l = L, .r = R } } } )

/** Patching entry */
struct patching_t {
  struct wand_expr_t *loc; ///< Location of patched sub-expressions
  struct wand_expr_t expr; ///< Original value of patched sub-expressions (for unpatching)
};

/** Type holding information for a solving step */
struct solve_step_t {
  size_t bind_depth; ///< Bind depth before binding variable
  size_t patch_depth; ///< Patch depth before propagation/normalization
  void *alloc_marker; ///< Allocation marker before propagation/normalization
  bool active; ///< Iteration active
  udomain_t iter; ///< Iteration state
  udomain_t seed; ///< Iteration random seed
  struct val_t bounds; ///< Iteration bounds
  struct constr_t *constr; ///< Normalized constraint after binding variable
};

/** Variable environment entry */
struct env_t {
  const char *key; ///< Key (identifier) of variable
  struct val_t *val; ///< Value of variable
  uint64_t fails; ///< Number of times this variable has failed
  struct solve_step_t *step; ///< Solving state
};

/** Types of objective functions */
enum objective_t {
  OBJ_ANY, ///< Find any solution
  OBJ_ALL, ///< Find all solutions
  OBJ_MIN, ///< Find solution that minimizes objective function
  OBJ_MAX  ///< Find solution that maximizes objective function
};

/** Types of ordering strategies */
enum order_t {
  ORDER_NONE,            ///< Pick the next variable in environment during solving
  ORDER_SMALLEST_DOMAIN, ///< Pick variable with the smallest domain
  ORDER_LARGEST_DOMAIN,  ///< Pick variable with the largest domain
  ORDER_SMALLEST_VALUE,  ///< Pick variable with lowest possible value
  ORDER_LARGEST_VALUE    ///< Pick variable with highest possible value
};

/** A struct holding shared information */
struct shared_t {
  sem_t semaphore; ///< Semaphore to synchronize accesses to shared data
  volatile uint32_t workers; ///< Number of active workers
  volatile uint32_t workers_id; ///< Number of generated workers
  volatile domain_t objective_best; ///< The current best objective value
  volatile uint64_t solutions; ///< Number of solutions found
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

/** Default size of allocation stack */
#define ALLOC_STACK_SIZE_DEFAULT (16*1024*1024)
/** Initialize the allocation stack */
void alloc_init(size_t size);
/** Deallocate memory occupied by the allocation stack */
void alloc_free(void);
/** Allocate memory on the allocation stack */
void *alloc(size_t size);
/** Deallocate memory on the allocation stack */
void dealloc(void *elem);

/** Hash value of a variable */
cache_tag_t hash_var(const struct val_t *var);

/** Mark a variable as dirty */
void cache_dirty(cache_tag_t tag);
/** Mark all variables as clean */
void cache_clean(void);
/** Check if a variable is marked dirty */
bool cache_is_dirty(cache_tag_t tag);

/** Default size of bind stack */
#define BIND_STACK_SIZE_DEFAULT (1024)
/** Initialize the binding structures */
void bind_init(size_t size);
/** Deallocate memory occupied by the bind stack */
void bind_free(void);
/** Bind a variable (at location loc) to a specific value */
size_t bind(struct val_t *loc, const struct val_t val);
/** Undo variable binds down to a given depth */
void unbind(size_t depth);

/** Default size of patch stack */
#define PATCH_STACK_SIZE_DEFAULT (16*1024)
/** Initialize the patching structures */
void patch_init(size_t size);
/** Deallocate memory occupied by the patch stack */
void patch_free(void);
/** Patch a wide-and sub-expression (at location loc) */
size_t patch(struct wand_expr_t *loc, const struct wand_expr_t expr);
/** Undo patches down to a given depth */
void unpatch(size_t depth);

/** Initialize a semaphore */
void sema_init(sem_t *sema);
/** Wait for a semaphore */
void sema_wait(sem_t *sema);
/** Release a semaphore */
void sema_post(sem_t *sema);

/** Evaluate a constraint */
const struct val_t eval(const struct constr_t *constr);
/** Fully normalize a constraint */
struct constr_t *normalize(struct constr_t *constr);
/** Perform a single normalizion step on a constraint */
struct constr_t *normal(struct constr_t *constr);
/** Propagate a result value to the terminal nodes of the constraint */
struct constr_t *propagate(struct constr_t *constr, struct val_t val);

/** Initialize objective function type */
void objective_init(enum objective_t o, volatile domain_t *best);
/** Get objective function type */
enum objective_t objective(void);
/** Check whether the objective value can be possibly better */
bool objective_better(void);
/** Get the current best objective value */
domain_t objective_best(void);
/** Get a pointer to the objective value variable */
struct val_t *objective_val(void);
/** Update the current best objective value */
void objective_update_best(void);
/** Update the objective value variable */
void objective_update_val(void);

/** Find solutions */
void solve(struct env_t *env, struct constr_t *constr);

/** Swap two environment entries (keeping step) */
void swap_env(struct env_t *env, size_t depth1, size_t depth2);

/** Whether to prefer failing variables as default */
#define STRATEGY_PREFER_FAILING_DEFAULT true
/** Set whether to prefer failing variables when ordering */
void strategy_prefer_failing_init(bool prefer_failing);
/** Get whether to prefer failing variables when ordering */
bool strategy_prefer_failing(void);

/** Whether to compute weights for initial ordering as default */
#define STRATEGY_COMPUTE_WEIGHTS_DEFAULT true
/** Set whether to compute weights for initial ordering */
void strategy_compute_weights_init(bool assign_weights);
/** Get whether to compute weights for initial ordering */
bool strategy_compute_weights(void);

/** Whether to enable restarts as default */
#define STRATEGY_RESTART_FREQUENCY_DEFAULT 100
/** Set whether to enable restarts */
void strategy_restart_frequency_init(uint64_t restart_frequency);
/** Get whether to enable restarts */
uint64_t strategy_restart_frequency(void);

/** Which ordering to use as default */
#define STRATEGY_ORDER_DEFAULT ORDER_NONE
/** Set the ordering to use when searching */
void strategy_order_init(enum order_t order);
/** Pick a variable according to the chosen strategy */
void strategy_pick_var(struct env_t *env, size_t depth);

/** How many workers to use by default */
#define WORKERS_MAX_DEFAULT 1
/** Initialize the shared data area */
void shared_init(int32_t workers_max);
/** Return pointer to the shared data area */
struct shared_t *shared(void);

/** Print value */
void print_val(FILE *file, const struct val_t val);
/** Print constraint */
void print_constr(FILE *file, const struct constr_t *constr);
/** Print variable environment/bindings */
void print_env(FILE *file, struct env_t *env);
/** Print a solution */
void print_solution(FILE *file, struct env_t *env);
/** Print an error message */
void print_error(const char *fmt, ...);
/** Print an error message and die */
void print_fatal(const char *fmt, ...);

/** Get the name of the program */
const char *main_name(void);

/** List of statistic counters */
#define STAT_LIST(F)                                                    \
    F(calls,     uint64_t, 0,        "CALLS: %lu, ")                    \
    F(cuts,      uint64_t, 0,        "CUTS: %lu, ")                     \
    F(props,     uint64_t, 0,        "PROPS: %lu, ")                    \
    F(restarts,  uint64_t, 0,        "RESTARTS: %lu, ")                 \
    F(depth_min, size_t,   SIZE_MAX, "DEPTH: %lu")                      \
    F(depth_max, size_t,   0,        "/%lu, ")                          \
    F(cut_depth, uint64_t, 0,        "AVG DEPTH: %f, ", (double)cut_depth/cuts) \
    F(alloc_max, size_t,   0,        "MEMORY: %lu")

/** Statistic counter variable */
#define STAT_EXTVAR(NAME, TYPE, RESET_VAL, ...)    \
  extern TYPE NAME;

/** Declare statistic vounter variables for list of statistic counters */
STAT_LIST(STAT_EXTVAR)

/** Functions provided for a statistic counter */
#define STAT_FUNCS(NAME, TYPE, RESET_VAL, ...)               \
  static inline TYPE stat_get_ ## NAME(void) { return NAME; }              \
  static inline void stat_set_ ## NAME(TYPE v) { NAME = v; }               \
  static inline void stat_reset_ ## NAME(void) { NAME = RESET_VAL; }       \
  static inline void stat_inc_ ## NAME(void)   { NAME++; }                 \
  static inline void stat_add_ ## NAME(TYPE v) { NAME += v; }              \
  static inline void stat_min_ ## NAME(TYPE v) { if (v < NAME) NAME = v; } \
  static inline void stat_max_ ## NAME(TYPE v) { if (v > NAME) NAME = v; }

/** Declare functions for list of statistic counters */
STAT_LIST(STAT_FUNCS)

/** Reset statistics */
void stats_init(void);
/** Print statistics */
void stats_print(FILE *file);
  
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
/** Error message when exceeding the maximum number of patches */
#define ERROR_MSG_TOO_MANY_PATCHES          "exceeded maximum number of patches"
/** Error message when encountering an invalid constraint type */
#define ERROR_MSG_INVALID_CONSTRAINT_TYPE   "invalid constraint type: %02x"
/** Error message when encountering an invalid operation */
#define ERROR_MSG_INVALID_OPERATION         "invalid operation: %02x"
/** Error message when encountering an invalid variable type */
#define ERROR_MSG_INVALID_VARIABLE_TYPE     "invalid variable type: %02x"
/** Error message when encountering an invalid objective function type */
#define ERROR_MSG_INVALID_OBJ_FUNC_TYPE     "invalid objective function type: %02x"
/** Error message when encountering invalid boolean arguments on the command line */
#define ERROR_MSG_INVALID_BOOL_ARG          "invalid boolean argument: %s"
/** Error message when encountering invalid integer arguments on the command line */
#define ERROR_MSG_INVALID_INT_ARG           "invalid integer argument: %s"
/** Error message when encountering invalid order arguments on the command line */
#define ERROR_MSG_INVALID_ORDER_ARG         "invalid order argument: %s"
/** Error message when encountering invalid size arguments on the command line */
#define ERROR_MSG_INVALID_SIZE_ARG          "invalid size argument: %s"
/** Error message when encountering invalid ordering strategy */
#define ERROR_MSG_INVALID_STRATEGY_ORDER    "invalid ordering strategy: %02x"

#endif
