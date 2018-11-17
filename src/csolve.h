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
  struct env_t *var; ///< Bound variable
  struct val_t val; ///< Original value of bound variable (for unbinding)
  size_t level; ///< Original binding level of variable (for unbinding)
  const struct wand_expr_t *clause; ///< Clause that inferred the binding
  struct binding_t *prev; ///< Previous binding for this variable
};

/** Propagation result type */
typedef int32_t prop_result_t;
/** Propagation failed */
#define PROP_ERROR (-1)
/** Propagation had no effect */
#define PROP_NONE 0

/** Propagation tag type */
typedef uint64_t prop_tag_t;

/** Type for a wide-and element */
struct wand_expr_t {
  struct constr_t *constr; ///< The constraint
  struct constr_t *orig; ///< The original/unpatched constrained
  prop_tag_t prop_tag; ///< Propagation tag
};

/** Type for conflict element */
struct confl_elem_t {
  struct val_t val; ///< Conflict value
  struct constr_t *var; ///< Conflict variable
};

/** Type representing a constraint */
struct constr_t {
  const struct constr_type_t *type; ///< Type of constraint node
  /** Union to hold either terminal node or expression node */
  union constr_union_t {
    /** Terminal node type */
    struct term_t {
      struct val_t val; ///< Value of terminal node
      struct env_t *env; ///< Link to environment of value
    } term;
    /** Expression node type */
    struct expr_t {
      struct constr_t *l; ///< Left child of expression
      struct constr_t *r; ///< Right child of expression, NULL for unary operators
    } expr; ///< Expression node
    /** Wide-and node type */
    struct wand_t {
      size_t length; ///< Number of sub-expressions
      struct wand_expr_t *elems; ///< Sub-expressions
    } wand; ///< Wide-and node
    /** Conflict node type */
    struct confl_t {
      size_t length; ///< Number of elements in conflict
      struct confl_elem_t *elems; ///< Conflict elements
    } confl; ///< Conflict node
  } constr; ///< Constraint
};

/** List of supported constraint types */
#define CONSTR_TYPE_LIST(F)                     \
  /** Terminal */                               \
  F(TERM, term, ' ')                            \
  /** Equality */                               \
  F(EQ,   eq,   '=')                            \
  /** Less-than */                              \
  F(LT,   lt,   '<')                            \
  /** Negation */                               \
  F(NEG,  neg,  '-')                            \
  /** Addition */                               \
  F(ADD,  add,  '+')                            \
  /** Multiplication */                         \
  F(MUL,  mul,  '*')                            \
  /** Logical not */                            \
  F(NOT,  not,  '!')                            \
  /** Logical and */                            \
  F(AND,  and,  '&')                            \
  /** Logical or */                             \
  F(OR,   or,   '|')                            \
  /** Wide and */                               \
  F(WAND, wand, 'A')                            \
  /** Conflict */                               \
  F(CONFL, confl, 'C')

/** Supported operators */
#define CONSTR_TYPE_OPS(UPNAME, NAME, OP)       \
  OP_ ## UPNAME = OP,
enum operator_t {
  CONSTR_TYPE_LIST(CONSTR_TYPE_OPS)
};

/** Constraint type */
struct constr_type_t {
  const struct val_t (*eval)(const struct constr_t *constr); ///< Evaluation function
  prop_result_t (*prop)(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause); ///< Propagation function
  struct constr_t * (*norm)(struct constr_t *constr); ///< Normalization function
  const enum operator_t op; ///< Operator
};

/** Supported constraint type variables */
#define CONSTR_TYPES_EXT(UPNAME, NAME, OP) \
  extern const struct constr_type_t CONSTR_ ## UPNAME;
CONSTR_TYPE_LIST(CONSTR_TYPES_EXT)

/** Checks whether constraint has a particular type */
#define IS_TYPE(T, C)                           \
  ((C)->type == &CONSTR_ ## T)

/** Checks whether constraint is a constant value */
static inline bool is_const(struct constr_t *c) {
  return IS_TYPE(TERM, c) && is_value(c->constr.term.val);
}

/** Create a terminal constraint */
#define CONSTRAINT_TERM(V)                                              \
  ((struct constr_t) {                                                  \
    .type = &CONSTR_TERM, .constr = { .term = { .val = V, .env = NULL } } } )

/** Create an expression constraint */
#define CONSTRAINT_EXPR(T, L, R)                                        \
  ((struct constr_t){                                                   \
    .type = &(CONSTR_ ## T), .constr = { .expr = { .l = L, .r = R } } } )

/** Create a wide-and constraint */
#define CONSTRAINT_WAND(L, E)                                           \
  ((struct constr_t) {                                                  \
    .type = &CONSTR_WAND, .constr = { .wand = { .length = L, .elems = E } } } )

/** Create a wide-and constraint */
#define CONSTRAINT_CONFL(L, E)                                          \
  ((struct constr_t) {                                                  \
    .type = &CONSTR_CONFL, .constr = { .confl = { .length = L, .elems = E } } } )

/** Patching entry */
struct patching_t {
  struct wand_expr_t *loc; ///< Location of patched sub-expressions
  struct constr_t *constr; ///< Original value of patched sub-expressions (for unpatching)
};

/** Type holding information for a solving step */
struct step_t {
  size_t bind_depth; ///< Bind depth before binding variable
  size_t patch_depth; ///< Patch depth before propagation/normalization
  void *alloc_marker; ///< Allocation marker before propagation/normalization
  struct env_t *var; ///< Environment of variable considered in this step
  bool active; ///< Iteration active
  udomain_t iter; ///< Iteration state
  udomain_t seed; ///< Iteration random seed
  struct val_t bounds; ///< Iteration bounds
};

/** Type to represent a list of clauses for propagation */
struct clause_list_t {
  size_t length; ///< Length of list
  struct wand_expr_t **elems; ///< Clause
};

/** Variable environment entry */
struct env_t {
  const char *key; ///< Key (identifier) of variable
  struct constr_t *val; ///< Value of variable
  struct binding_t *binds; ///< bindings of this variable
  struct clause_list_t clauses; ///< Clauses affected by this value
  size_t order; ///< Position in variable ordering
  int64_t prio; ///< Priority of this variable
  size_t level; ///< Assignment level of this variable
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
  volatile bool     timeout; ///< Whether timeout has occurred
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
#define ALLOC_STACK_SIZE_DEFAULT (128*1024*1024)
/** Initialize the allocation stack */
void alloc_init(size_t size);
/** Deallocate memory occupied by the allocation stack */
void alloc_free(void);
/** Allocate memory on the allocation stack */
void *alloc(size_t size);
/** Deallocate memory on the allocation stack */
void dealloc(void *elem);

/** Default size of bind stack */
#define BIND_STACK_SIZE_DEFAULT (1024*1024)
/** Initialize the binding structures */
void bind_init(size_t size);
/** Deallocate memory occupied by the bind stack */
void bind_free(void);
/** Make current bindings permanent */
void bind_commit(void);
/** Get the current bind depth */
size_t bind_depth(void);
/** Set the level for subsequent binds */
void bind_level_set(size_t level);
/** Set the current bind level */
size_t bind_level_get();
/** Bind a variable to a specific value */
void bind(struct env_t *var, const struct val_t val, const struct wand_expr_t *clause);
/** Undo variable binds down to a given depth */
void unbind(size_t depth);

/** Default size of patch stack */
#define PATCH_STACK_SIZE_DEFAULT (1024*1024)
/** Initialize the patching structures */
void patch_init(size_t size);
/** Deallocate memory occupied by the patch stack */
void patch_free(void);
/** Make current patches permanent */
void patch_commit(void);
/** Patch a wide-and sub-expression (at location loc) */
size_t patch(struct wand_expr_t *loc, struct constr_t *constr);
/** Undo patches down to a given depth */
void unpatch(size_t depth);

/** Initialize a semaphore */
void sema_init(sem_t *sema);
/** Wait for a semaphore */
void sema_wait(sem_t *sema);
/** Release a semaphore */
void sema_post(sem_t *sema);

/** Add an element to a clause list */
void clause_list_append(struct clause_list_t *list, struct wand_expr_t *elem);

/** Evaluation functions for different constraint types */
#define CONSTR_TYPE_EVAL_FUNCS(UPNAME, NAME, OP)                    \
  const struct val_t eval_ ## NAME(const struct constr_t *constr);
CONSTR_TYPE_LIST(CONSTR_TYPE_EVAL_FUNCS)

/** Propagation functions for different constraint types */
#define CONSTR_TYPE_PROP_FUNCS(UPNAME, NAME, OP)                    \
  prop_result_t propagate_ ## NAME(struct constr_t *constr, const struct val_t val, const struct wand_expr_t *clause);
CONSTR_TYPE_LIST(CONSTR_TYPE_PROP_FUNCS)

/** Normalization functions for different constraint types */
#define CONSTR_TYPE_NORM_FUNCS(UPNAME, NAME, OP)                    \
  struct constr_t *normal_ ## NAME(struct constr_t *constr);
CONSTR_TYPE_LIST(CONSTR_TYPE_NORM_FUNCS)

/** Normalize a constraint */
struct constr_t *normalize(struct constr_t *constr);

/** Propagate "true" to the terminal nodes of the constraint */
prop_result_t propagate(struct constr_t *constr);
/** Propagate updates to a list of clauses */
prop_result_t propagate_clauses(const struct clause_list_t *clauses);

/** Default size of conflict allocation stack */
#define CONFLICT_ALLOC_STACK_SIZE_DEFAULT (128*1024*1024)
/** Initialize the conflict allocation stack */
void conflict_alloc_init(size_t size);
/** Deallocate memory occupied by the conflict allocation stack */
void conflict_alloc_free(void);
/** Create a conflict clause */
void conflict_create(struct env_t *var, const struct wand_expr_t *clause);
/** Get level of last generated conflict */
size_t conflict_level(void);
/** Get variable of last generated conflict */
struct env_t *conflict_var(void);
/** Reset information about last generated conflict */
void conflict_reset(void);

/** Initialize objective function type */
void objective_init(enum objective_t o, volatile domain_t *best);
/** Get objective function type */
enum objective_t objective(void);
/** Check whether the objective value can be possibly better */
bool objective_better(void);
/** Get the current best objective value */
domain_t objective_best(void);
/** Get a pointer to the objective value variable */
struct constr_t *objective_val(void);
/** Update the current best objective value */
void objective_update_best(void);
/** Update the objective value variable */
void objective_update_val(void);

/** Find solutions */
void solve(size_t size, struct env_t *env, struct constr_t *constr);

/** Whether to create conflict clauses as default */
#define STRATEGY_CREATE_CONFLICTS_DEFAULT true
/** Set whether to create conflict clauses */
void strategy_create_conflicts_init(bool create_conflicts);
/** Get whether to create conflict clauses */
bool strategy_create_conflicts(void);

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
/** Initialize the variable ordering */
void strategy_var_order_init(size_t size, struct env_t *env);
/** Free memory used by the variable ordering */
void strategy_var_order_free(void);
/** Pick a variable according to the chosen strategy */
struct env_t *strategy_var_order_pop(void);
/** Put back variable into ordering */
void strategy_var_order_push(struct env_t *e);
/** Update position of variable in ordering */
void strategy_var_order_update(struct env_t *e);

/** How many workers to use by default */
#define WORKERS_MAX_DEFAULT 1
/** Initialize the shared data area */
void shared_init(uint32_t workers_max);
/** Return pointer to the shared data area */
struct shared_t *shared(void);

/** Default timeout */
#define TIME_MAX_DEFAULT 0
/** Initialize the timeout data */
void timeout_init(uint32_t time_max);

/** Print value */
void print_val(FILE *file, const struct val_t val);
/** Print constraint */
void print_constr(FILE *file, const struct constr_t *constr);
/** Print variable environment/bindings */
void print_env(FILE *file, size_t size, struct env_t *env);
/** Print a solution */
void print_solution(FILE *file, size_t size, struct env_t *env);
/** Print an error message */
void print_error(const char *fmt, ...);
/** Print an error message and die */
void print_fatal(const char *fmt, ...);

/** Get the name of the program */
const char *main_name(void);

/** List of statistic counters */
#define STAT_LIST(F)                                                    \
  F(calls,      uint64_t, 0,        "CALLS: %lu, ")                     \
  F(cuts,       uint64_t, 0,        "CUTS: %lu, ")                      \
  F(props,      uint64_t, 0,        "PROPS: %lu, ")                     \
  F(confl,      uint64_t, 0,        "CONFL: %lu, ")                     \
  F(restarts,   uint64_t, 0,        "RESTARTS: %lu, ")                  \
  F(level_min,  size_t,   SIZE_MAX, "LEVEL: %lu")                       \
  F(level_max,  size_t,   0,        "/%lu, ")                           \
  F(cut_level,  uint64_t, 0,        "AVG LEVEL: %f, ", (double)cut_level/cuts) \
  F(alloc_max,  size_t,   0,        "MEM: %lu, ")                       \
  F(calloc_max, size_t,   0,        "CMEM: %lu")                        \

/** Statistic counter variable */
#define STAT_EXTVAR(NAME, TYPE, RESET_VAL, ...)    \
  extern TYPE NAME;

/** Declare statistic vounter variables for list of statistic counters */
STAT_LIST(STAT_EXTVAR)

/** Functions provided for a statistic counter */
#define STAT_FUNCS(NAME, TYPE, RESET_VAL, ...)                          \
  static inline TYPE stat_get_ ## NAME(void) { return NAME; }           \
  static inline void stat_set_ ## NAME(TYPE v) { NAME = v; }            \
  static inline void stat_reset_ ## NAME(void) { NAME = RESET_VAL; }    \
  static inline void stat_inc_ ## NAME(void)   { NAME++; }              \
  static inline void stat_add_ ## NAME(TYPE v) { NAME += v; }           \
  static inline void stat_min_ ## NAME(TYPE v) { if (v < NAME) { NAME = v; } } \
  static inline void stat_max_ ## NAME(TYPE v) { if (v > NAME) { NAME = v; } }

/** Declare functions for list of statistic counters */
STAT_LIST(STAT_FUNCS)

/** Reset statistics */
void stats_init(void);
/** Print statistics */
void stats_print(FILE *file);

/** Default statistics printing frequency */
#define STATS_FREQUENCY_DEFAULT 10000
/** Set statistics printing frequency */
void stats_frequency_init(uint64_t freq);
/** Get statistics printing frequency */
uint64_t stats_frequency(void);

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
/** Error message when an unbounded variable is encountered */
#define ERROR_MSG_UNBOUNDED_VARIABLE        "unbounded variable: %s"

#endif
