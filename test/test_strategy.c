#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace strategy {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/strategy.c"

class Mock {
 public:
  MOCK_METHOD3(swap_env, void(struct env_t *, size_t, size_t));
  MOCK_METHOD1(print_fatal, void (const char *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, struct val_t(const struct constr_t *)); \
  MOCK_METHOD3(propagate_ ## NAME, prop_result_t(struct constr_t *, struct val_t, const struct wand_expr_t *)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

void swap_env(struct env_t *env, size_t depth1, size_t depth2) {
  MockProxy->swap_env(env, depth1, depth2);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
struct val_t eval_ ## NAME(const struct constr_t *constr) {       \
  return MockProxy->eval_ ## NAME(constr);                              \
}                                                                       \
prop_result_t propagate_ ## NAME(struct constr_t *constr, struct val_t val, const struct wand_expr_t *clause) { \
  return MockProxy->propagate_ ## NAME(constr, val, clause);            \
}                                                                       \
struct constr_t *normal_ ## NAME(struct constr_t *constr) {             \
  return MockProxy->normal_ ## NAME(constr);                            \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

TEST(CreateConflicts, Init) {
  strategy_create_conflicts_init(true);
  EXPECT_EQ(true, _create_conflicts);
  strategy_create_conflicts_init(false);
  EXPECT_EQ(false, _create_conflicts);
}

TEST(CreateConflicts, Get) {
  _create_conflicts = true;
  EXPECT_EQ(true, strategy_create_conflicts());
  _create_conflicts = false;
  EXPECT_EQ(false, strategy_create_conflicts());
}

TEST(PreferFailing, Init) {
  strategy_prefer_failing_init(true);
  EXPECT_EQ(true, _prefer_failing);
  strategy_prefer_failing_init(false);
  EXPECT_EQ(false, _prefer_failing);
}

TEST(PreferFailing, Get) {
  _prefer_failing = true;
  EXPECT_EQ(true, strategy_prefer_failing());
  _prefer_failing = false;
  EXPECT_EQ(false, strategy_prefer_failing());
}

TEST(ComputeWeigths, Init) {
  strategy_compute_weights_init(true);
  EXPECT_EQ(true, _compute_weights);
  strategy_compute_weights_init(false);
  EXPECT_EQ(false, _compute_weights);
}

TEST(ComputeWeigths, Get) {
  _compute_weights = true;
  EXPECT_EQ(true, strategy_compute_weights());
  _compute_weights = false;
  EXPECT_EQ(false, strategy_compute_weights());
}

TEST(RestartFrequency, Init) {
  strategy_restart_frequency_init(17);
  EXPECT_EQ(17U, _restart_frequency);
  strategy_restart_frequency_init(23);
  EXPECT_EQ(23U, _restart_frequency);
}

TEST(RestartFrequency, Get) {
  _restart_frequency = 17;
  EXPECT_EQ(17U, strategy_restart_frequency());
  _restart_frequency = 23;
  EXPECT_EQ(23U, strategy_restart_frequency());
}

TEST(Order, Init) {
  strategy_order_init(ORDER_SMALLEST_DOMAIN);
  EXPECT_EQ(ORDER_SMALLEST_DOMAIN, _order);
  strategy_order_init(ORDER_LARGEST_VALUE);
  EXPECT_EQ(ORDER_LARGEST_VALUE, _order);
}

TEST(VarCmp, SmallestDomain) {
  _order = ORDER_SMALLEST_DOMAIN;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(5, 7));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  _prefer_failing = false;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[1]), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_GT(strategy_var_cmp(&env[2], &env[1]), 0);
}

TEST(VarCmp, LargestDomain) {
  _order = ORDER_LARGEST_DOMAIN;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  _prefer_failing = false;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[1]), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_GT(strategy_var_cmp(&env[2], &env[1]), 0);
}

TEST(VarCmp, SmallestValue) {
  _order = ORDER_SMALLEST_VALUE;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  _prefer_failing = false;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[1]), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_GT(strategy_var_cmp(&env[2], &env[1]), 0);
}

TEST(VarCmp, LargestValue) {
  _order = ORDER_LARGEST_VALUE;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  _prefer_failing = false;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[1]), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_GT(strategy_var_cmp(&env[2], &env[1]), 0);
}

TEST(VarCmp, None) {
  _order = ORDER_NONE;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  _prefer_failing = false;
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[1]), 0);

  _prefer_failing = true;
  EXPECT_LT(strategy_var_cmp(&env[0], &env[1]), 0);
  EXPECT_GT(strategy_var_cmp(&env[1], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[0], &env[0]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[1], &env[1]), 0);
  EXPECT_EQ(strategy_var_cmp(&env[2], &env[2]), 0);
  EXPECT_LT(strategy_var_cmp(&env[1], &env[2]), 0);
  EXPECT_GT(strategy_var_cmp(&env[2], &env[1]), 0);
}

TEST(VarCmp, Error) {
  _order = (order_t)0x1337;
  _prefer_failing = false;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 5, .level = 0 };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_STRATEGY_ORDER)).Times(1);
  strategy_var_cmp(&env[2], &env[1]);
  delete(MockProxy);
}

TEST(VarOrder, Parent) {
  EXPECT_EQ(parent(1), 0);
  EXPECT_EQ(parent(2), 0);
  EXPECT_EQ(parent(3), 1);
  EXPECT_EQ(parent(4), 1);
  EXPECT_EQ(parent(5), 2);
  EXPECT_EQ(parent(6), 2);
}

TEST(VarOrder, Left) {
  EXPECT_EQ(left(0), 1);
  EXPECT_EQ(left(1), 3);
  EXPECT_EQ(left(2), 5);
  EXPECT_EQ(left(3), 7);
}

TEST(VarOrder, Right) {
  EXPECT_EQ(right(0), 2);
  EXPECT_EQ(right(1), 4);
  EXPECT_EQ(right(2), 6);
  EXPECT_EQ(right(3), 8);
}

TEST(VarOrder, Swap) {
  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 1, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 2, .prio = 5, .level = 0 };

  struct env_t *v[3];
  _var_order = v;
  _var_order[0] = &env[0];
  _var_order[1] = &env[1];
  _var_order[2] = &env[2];

  MockProxy = new Mock();
  strategy_var_order_swap(0, 2);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(0U, env[2].order);
  EXPECT_EQ(_var_order[1], &env[1]);
  EXPECT_EQ(1U, env[1].order);
  EXPECT_EQ(_var_order[2], &env[0]);
  EXPECT_EQ(2U, env[0].order);
  delete(MockProxy);
}

TEST(VarOrder, Up) {
  _order = ORDER_NONE;
  _prefer_failing = true;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 1, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 2, .prio = 5, .level = 0 };

  _var_order_size = 3;

  struct env_t *v[_var_order_size];
  _var_order = v;
  _var_order[0] = &env[0];
  _var_order[1] = &env[1];
  _var_order[2] = &env[2];

  MockProxy = new Mock();
  strategy_var_order_up(1);
  EXPECT_EQ(_var_order[0], &env[1]);
  EXPECT_EQ(_var_order[1], &env[0]);
  EXPECT_EQ(_var_order[2], &env[2]);
  delete(MockProxy);

  MockProxy = new Mock();
  strategy_var_order_up(2);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(_var_order[1], &env[0]);
  EXPECT_EQ(_var_order[2], &env[1]);
  delete(MockProxy);

  MockProxy = new Mock();
  strategy_var_order_up(1);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(_var_order[1], &env[0]);
  EXPECT_EQ(_var_order[2], &env[1]);
  delete(MockProxy);
}

TEST(VarOrder, Down) {
  _order = ORDER_NONE;
  _prefer_failing = true;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 1, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 2, .prio = 5, .level = 0 };

  _var_order_size = 3;

  struct env_t *v[_var_order_size];
  _var_order = v;
  _var_order[0] = &env[0];
  _var_order[1] = &env[1];
  _var_order[2] = &env[2];

  MockProxy = new Mock();
  strategy_var_order_down(1);
  EXPECT_EQ(_var_order[0], &env[0]);
  EXPECT_EQ(_var_order[1], &env[1]);
  EXPECT_EQ(_var_order[2], &env[2]);
  delete(MockProxy);

  MockProxy = new Mock();
  strategy_var_order_down(2);
  EXPECT_EQ(_var_order[0], &env[0]);
  EXPECT_EQ(_var_order[1], &env[1]);
  EXPECT_EQ(_var_order[2], &env[2]);
  delete(MockProxy);

  MockProxy = new Mock();
  strategy_var_order_down(0);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(_var_order[1], &env[1]);
  EXPECT_EQ(_var_order[2], &env[0]);
  delete(MockProxy);
}

TEST(VarOrder, Push) {
  _order = ORDER_NONE;
  _prefer_failing = true;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 5, .level = 0 };

  _var_order_size = 0;
  struct env_t *v[3];
  _var_order = v;

  strategy_var_order_push(&env[0]);
  EXPECT_EQ(_var_order_size, 1);
  EXPECT_EQ(_var_order[0], &env[0]);

  strategy_var_order_push(&env[1]);
  EXPECT_EQ(_var_order_size, 2);
  EXPECT_EQ(_var_order[0], &env[1]);
  EXPECT_EQ(_var_order[1], &env[0]);

  strategy_var_order_push(&env[2]);
  EXPECT_EQ(_var_order_size, 3);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(_var_order[1], &env[0]);
  EXPECT_EQ(_var_order[2], &env[1]);
}

TEST(VarOrder, Pop) {
  _order = ORDER_NONE;
  _prefer_failing = true;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = SIZE_MAX, .prio = 5, .level = 0 };

  _var_order_size = 3;
  struct env_t *v[_var_order_size];
  _var_order = v;
  _var_order[0] = &env[2];
  _var_order[1] = &env[0];
  _var_order[2] = &env[1];

  EXPECT_EQ(strategy_var_order_pop(), &env[2]);
  EXPECT_EQ(strategy_var_order_pop(), &env[1]);
  EXPECT_EQ(strategy_var_order_pop(), &env[0]);
}

TEST(VarOrder, Update) {
  _order = ORDER_NONE;
  _prefer_failing = true;

  struct env_t env[3];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 1, .prio = 4, .level = 0 };
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[2] = { .key = "c", .val = &c, .binds = NULL,
             .clauses = { .length = 0, .elems = NULL },
             .order = 2, .prio = 5, .level = 0 };

  _var_order_size = 3;
  struct env_t *v[_var_order_size];
  _var_order = v;
  _var_order[0] = &env[0];
  _var_order[1] = &env[1];
  _var_order[2] = &env[2];

  strategy_var_order_update(&env[1]);
  EXPECT_EQ(_var_order[0], &env[2]);
  EXPECT_EQ(_var_order[1], &env[0]);
  EXPECT_EQ(_var_order[2], &env[1]);
}

}
