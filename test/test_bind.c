#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace bind {
#include "../src/csolve.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD2(eval, const struct val_t(const struct env_t *, const struct constr_t *));
  MOCK_METHOD2(normalize, struct constr_t *(struct env_t *, struct constr_t *));
  MOCK_METHOD3(propagate, struct constr_t *(struct env_t *, struct constr_t *, struct val_t));

  MOCK_METHOD0(eval_cache_invalidate, void(void));

  MOCK_METHOD0(objective, enum objective_t(void));
  MOCK_METHOD2(objective_better, bool(struct env_t *, struct constr_t *));
  MOCK_METHOD1(objective_update, void(struct val_t));
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD2(objective_optimize, struct constr_t *(struct env_t *, struct constr_t *));

  MOCK_METHOD2(print_solution, void(FILE *, struct env_t *));
};

Mock *MockProxy;

const struct val_t eval(const struct env_t *env, const struct constr_t *constr) {
  return MockProxy->eval(env, constr);
}

struct constr_t *normalize(struct env_t *env, struct constr_t *constr) {
  return MockProxy->normalize(env, constr);
}

struct constr_t *propagate(struct env_t *env, struct constr_t *constr, struct val_t val) {
  return MockProxy->propagate(env, constr, val);
}

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

enum objective_t objective(void) {
  return MockProxy->objective();
}

bool objective_better(struct env_t *env, struct constr_t *obj) {
  return MockProxy->objective_better(env, obj);
}

void objective_update(struct val_t obj) {
  MockProxy->objective_update(obj);
}

domain_t objective_best(void) {
  return MockProxy->objective_best();
}

struct constr_t *objective_optimize(struct env_t *env, struct constr_t *obj) {
  return MockProxy->objective_optimize(env, obj);
}

void print_solution(FILE *file, struct env_t *env) {
  return MockProxy->print_solution(file, env);
}

TEST(Bind, Success) {
  struct val_t loc = INTERVAL(0, 100);

  bind_depth = 23;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(1);
  EXPECT_EQ(23, bind(&loc, VALUE(17)));
  EXPECT_EQ(24, bind_depth);
  EXPECT_EQ(loc, VALUE(17));
  delete(MockProxy);
}

TEST(Bind, Fail) {
  std::string output;
  struct val_t loc = INTERVAL(0, 100);

  bind_depth = 23;
  testing::internal::CaptureStderr();
  EXPECT_EQ(MAX_BINDS, bind(NULL, VALUE(17)));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: cannot bind NULL\n");

  bind_depth = MAX_BINDS;
  testing::internal::CaptureStderr();
  EXPECT_EQ(MAX_BINDS, bind(&loc, VALUE(17)));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: exceeded maximum number of binds\n");
}

TEST(Unbind, Sucess) {
  struct val_t loc1 = INTERVAL(0, 100);
  struct val_t loc2 = INTERVAL(17, 23);

  bind_depth = 17;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(2);
  bind(&loc1, VALUE(42));
  bind(&loc2, VALUE(23));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(2);
  unbind(17);
  EXPECT_EQ(loc1, INTERVAL(0, 100));
  EXPECT_EQ(loc2, INTERVAL(17, 23));
  EXPECT_EQ(17, bind_depth);
  delete(MockProxy);
}

} // end namespace
