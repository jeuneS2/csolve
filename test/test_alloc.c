#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace alloc {
#include "../src/csolve.c"

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

TEST(Alloc, Success) {
  alloc_max = 64;
  alloc_stack_pointer = 32;
  size_t ptr = alloc_stack_pointer;

  EXPECT_EQ(&alloc_stack[ptr], alloc(23));
  ptr += 23+1;
  EXPECT_EQ(ptr, alloc_stack_pointer);
  EXPECT_EQ(64, alloc_max);

  EXPECT_EQ(&alloc_stack[ptr], alloc(17));
  ptr += 17+7;
  EXPECT_EQ(ptr, alloc_stack_pointer);
  EXPECT_EQ(ptr, alloc_max);
}

TEST(Alloc, Fail) {
  std::string output;

  alloc_stack_pointer = 32;
  testing::internal::CaptureStderr();
  EXPECT_EQ(NULL, alloc(ALLOC_STACK_SIZE));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: out of memory\n");
}

TEST(Dealloc, Success) {
  alloc_stack_pointer = 64;
  dealloc(&alloc_stack[24]);
  EXPECT_EQ(24, alloc_stack_pointer);
}

TEST(Dealloc, Fail) {
  std::string output;

  alloc_stack_pointer = 64;

  testing::internal::CaptureStderr();
  dealloc(&alloc_stack[ALLOC_ALIGNMENT-1]);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");

  testing::internal::CaptureStderr();
  dealloc(&alloc_stack[128]);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");

  testing::internal::CaptureStderr();
  dealloc(NULL);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");
}

} // end namespace
