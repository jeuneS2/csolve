#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace normalize {
#include "../src/arith.c"
#include "../src/normalize.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD2(eval, const struct val_t(const struct env_t *, const struct constr_t *));
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD3(update_expr, struct constr_t *(struct constr_t *, struct constr_t *, struct constr_t *));
  MOCK_METHOD2(update_unary_expr, struct constr_t *(struct constr_t *, struct constr_t *));
};

Mock *MockProxy;

const struct val_t eval(const struct env_t *env, const struct constr_t *constr) {
  return MockProxy->eval(env, constr);
}

void *alloc(size_t size) {
  return MockProxy->alloc(size);
}

struct constr_t *update_expr(struct constr_t *constr, struct constr_t *l, struct constr_t *r) {
  return MockProxy->update_expr(constr, l, r);
}

struct constr_t *update_unary_expr(struct constr_t *constr, struct constr_t *l) {
  return MockProxy->update_unary_expr(constr, l);
}

TEST(NormalizeNeg, Basic) {
  struct val_t a = VALUE(17);

  struct env_t env [2] = { { "a", &a },
                           { NULL, NULL } };

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_neg(env, &X));
  delete(MockProxy);
}

TEST(NormalizeNot, Basic) {
  struct val_t a = VALUE(1);

  struct env_t env [2] = { { "a", &a },
                           { NULL, NULL } };

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_not(env, &X));
  delete(MockProxy);
}

} // end namespace
