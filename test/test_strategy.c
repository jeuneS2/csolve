#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace strategy {
#include "../src/arith.c"
#include "../src/strategy.c"

class Mock {
 public:
  MOCK_METHOD3(swap_env, void(struct env_t *, size_t, size_t));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

void swap_env(struct env_t *env, size_t depth1, size_t depth2) {
  MockProxy->swap_env(env, depth1, depth2);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
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
  EXPECT_EQ(17, _restart_frequency);
  strategy_restart_frequency_init(23);
  EXPECT_EQ(23, _restart_frequency);
}

TEST(RestartFrequency, Get) {
  _restart_frequency = 17;
  EXPECT_EQ(17, strategy_restart_frequency());
  _restart_frequency = 23;
  EXPECT_EQ(23, strategy_restart_frequency());
}

TEST(Order, Init) {
  strategy_order_init(ORDER_SMALLEST_DOMAIN);
  EXPECT_EQ(ORDER_SMALLEST_DOMAIN, _order);
  strategy_order_init(ORDER_LARGEST_VALUE);
  EXPECT_EQ(ORDER_LARGEST_VALUE, _order);
}

TEST(PickVarCmp, SmallestDomain) {
  _order = ORDER_SMALLEST_DOMAIN;

  struct env_t env[4];

  struct val_t a = INTERVAL(5, 7);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  _prefer_failing = false;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 1), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 2, 1), 0);
}

TEST(PickVarCmp, LargestDomain) {
  _order = ORDER_LARGEST_DOMAIN;

  struct env_t env[4];

  struct val_t a = INTERVAL(5, 27);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  _prefer_failing = false;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 1), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 2, 1), 0);
}

TEST(PickVarCmp, SmallestValue) {
  _order = ORDER_SMALLEST_VALUE;

  struct env_t env[4];

  struct val_t a = INTERVAL(1, 27);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  _prefer_failing = false;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 1), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 2, 1), 0);
}

TEST(PickVarCmp, LargestValue) {
  _order = ORDER_LARGEST_VALUE;

  struct env_t env[4];

  struct val_t a = INTERVAL(1, 27);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  _prefer_failing = false;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 1), 0);

  _prefer_failing = true;
  EXPECT_GT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 2, 1), 0);
}

TEST(PickVarCmp, None) {
  _order = ORDER_NONE;

  struct env_t env[4];

  struct val_t a = INTERVAL(1, 27);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  _prefer_failing = false;
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 1), 0);

  _prefer_failing = true;
  EXPECT_LT(strategy_pick_var_cmp(env, 0, 1), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 1, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 0, 0), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 1, 1), 0);
  EXPECT_EQ(strategy_pick_var_cmp(env, 2, 2), 0);
  EXPECT_LT(strategy_pick_var_cmp(env, 1, 2), 0);
  EXPECT_GT(strategy_pick_var_cmp(env, 2, 1), 0);
}

TEST(PickVarCmp, Error) {
  _order = (order_t)0x1337;
  _prefer_failing = false;

  struct env_t env[4];

  struct val_t a = INTERVAL(1, 27);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_STRATEGY_ORDER)).Times(1);
  strategy_pick_var_cmp(env, 2, 1);
  delete(MockProxy);
}

TEST(PickVar, Basic) {
  _order = ORDER_SMALLEST_DOMAIN;
  _prefer_failing = true;

  struct env_t env[4];

  struct val_t a = INTERVAL(5, 7);
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = NULL };
  struct val_t b = INTERVAL(3, 17);
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = NULL };
  struct val_t c = INTERVAL(3, 17);
  env[2] = { .key = "c", .val = &c, .fails = 5, .step = NULL };
  env[3] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, swap_env(env, 1, 2)).Times(1);
  strategy_pick_var(env, 1);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, swap_env(env, 2, 2)).Times(1);
  strategy_pick_var(env, 2);
  delete(MockProxy);

  MockProxy = new Mock();
  strategy_pick_var(env, 3);
  delete(MockProxy);
}

}
