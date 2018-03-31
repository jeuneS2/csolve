#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace normalize {
#include "../src/arith.c"
#include "../src/normalize.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(eval, const struct val_t(const struct constr_t *));
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD3(update_expr, struct constr_t *(struct constr_t *, struct constr_t *, struct constr_t *));
  MOCK_METHOD2(update_unary_expr, struct constr_t *(struct constr_t *, struct constr_t *));
  MOCK_METHOD2(print_error, void (const char *, va_list));
};

Mock *MockProxy;

const struct val_t eval(const struct constr_t *constr) {
  return MockProxy->eval(constr);
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

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

TEST(NormalizeEval, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);
  struct val_t c;

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;
  struct constr_t Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(VALUE(27)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct val_t)))
    .Times(1)
    .WillOnce(::testing::Return(&c));
  EXPECT_EQ(&Y, normal_eval(&X));
  EXPECT_EQ(true, is_const(&Y));
  EXPECT_EQ(&c, Y.constr.term);
  EXPECT_EQ(27, get_lo(*Y.constr.term));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(23, 42)));
  EXPECT_EQ(&X, normal_eval(&X));
  delete(MockProxy);
}

TEST(NormalizeEq, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_eq(&X));
  delete(MockProxy);
}

TEST(NormalizeLt, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_lt(&X));
  delete(MockProxy);
}

TEST(NormalizeNeg, Basic) {
  struct val_t a = INTERVAL(17, 18);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_neg(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NEG, &X, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(INTERVAL(-18,-17)));
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&A, normal_neg(&Y));
  delete(MockProxy);
}

TEST(NormalizeAdd, Basic) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(0);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_add(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &B, &A);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_add(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &C);
  EXPECT_EQ(&A, normal_add(&X));
  delete(MockProxy);
}

TEST(NormalizeAdd, Pattern) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = INTERVAL(23, 42);
  struct val_t c = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &C);
  Y = CONSTRAINT_EXPR(OP_ADD, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&Y, &X, &C))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_add(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &C);
  Y = CONSTRAINT_EXPR(OP_ADD, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&Y, &A, &X))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_add(&Y));
  delete(MockProxy);
}

TEST(NormalizeMul, Basic) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_mul(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &B, &A);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_mul(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &C);
  EXPECT_EQ(&A, normal_mul(&X));
  delete(MockProxy);
}

TEST(NormalizeMul, Pattern) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = INTERVAL(23, 42);
  struct val_t c = VALUE(2);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &C);
  Y = CONSTRAINT_EXPR(OP_MUL, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&Y, &X, &C))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_mul(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &C);
  Y = CONSTRAINT_EXPR(OP_MUL, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, update_expr(&Y, &A, &X))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_mul(&Y));
  delete(MockProxy);
}

TEST(NormalizeNot, Basic) {
  struct val_t a = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_not(&X));
  delete(MockProxy);
}

TEST(NormalizeAnd, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(0);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &A);
  EXPECT_EQ(&A, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(&B, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &B, &A);
  EXPECT_EQ(&B, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &C);
  EXPECT_EQ(&C, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &C, &A);
  EXPECT_EQ(&C, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &B, &C);
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &C, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &C, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);
}

TEST(NormalizeOr, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &A);
  EXPECT_EQ(&A, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(&B, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &B, &A);
  EXPECT_EQ(&B, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &C);
  EXPECT_EQ(&C, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &C, &A);
  EXPECT_EQ(&C, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &B, &C);
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &C))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &C, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &C, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);
}

TEST(Normalize, Basic) {
  struct val_t a = INTERVAL(-17,23);
  struct val_t b = INTERVAL(-42,77);
  struct val_t c;

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &A);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct val_t)))
    .Times(1)
    .WillOnce(::testing::Return(&c));
  EXPECT_EQ(&Y, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);
}

TEST(Normalize, Errors) {
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, eval(&X)).Times(1);
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_EQ(&X, normal(&X));
  delete(MockProxy);
}

TEST(Normalize, Loop) {
  struct val_t a = INTERVAL(17,23);
  struct val_t c;

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &A);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct val_t)))
    .Times(1)
    .WillOnce(::testing::Return(&c));
  EXPECT_EQ(&Y, normalize(&X));
  delete(MockProxy);
}

} // end namespace
