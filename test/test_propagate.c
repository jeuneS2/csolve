#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace propagate {
#include "../src/arith.c"
#include "../src/eval.c"
#include "../src/propagate.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD2(bind, size_t(struct val_t *, const struct val_t));
  MOCK_METHOD3(update_expr, struct constr_t *(struct constr_t *, struct constr_t *, struct constr_t *));
  MOCK_METHOD2(update_unary_expr, struct constr_t *(struct constr_t *, struct constr_t *));
  MOCK_METHOD2(print_error, void (const char *, va_list));
};

Mock *MockProxy;

size_t bind(struct val_t *loc, const struct val_t val) {
  return MockProxy->bind(loc, val);
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

TEST(PropagateTerm, Value) {
  struct val_t a = VALUE(23);
  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };

  MockProxy = new Mock();
  EXPECT_EQ(&A, propagate_term(&A, VALUE(23)));
  EXPECT_EQ(NULL, propagate_term(&A, VALUE(17)));
  EXPECT_EQ(NULL, propagate_term(&A, VALUE(42)));
  EXPECT_EQ(&A, propagate_term(&A, INTERVAL(17, 42)));
  EXPECT_EQ(NULL, propagate_term(&A, INTERVAL(0, 17)));
  EXPECT_EQ(NULL, propagate_term(&A, INTERVAL(42, 100)));
  delete(MockProxy);
}

TEST(PropagateTerm, Interval) {
  struct val_t a = INTERVAL(23, 24);
  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(23))).Times(1);
  EXPECT_EQ(&A, propagate_term(&A, VALUE(23)));
  delete(MockProxy);

  EXPECT_EQ(NULL, propagate_term(&A, VALUE(17)));
  EXPECT_EQ(NULL, propagate_term(&A, VALUE(42)));
  EXPECT_EQ(&A, propagate_term(&A, INTERVAL(17, 42)));

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(23))).Times(1);
  EXPECT_EQ(&A, propagate_term(&A, INTERVAL(-1, 23)));
  delete(MockProxy);

  EXPECT_EQ(NULL, propagate_term(&A, INTERVAL(0, 17)));
  EXPECT_EQ(NULL, propagate_term(&A, INTERVAL(42, 100)));
}

TEST(PropagateEq, Value) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_eq(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_eq(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_eq(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateEq, Interval) {
  struct val_t a = INTERVAL(0, 23);
  struct val_t b = INTERVAL(17, 42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_eq(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_eq(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, INTERVAL(17, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, INTERVAL(17, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_eq(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateLt, Value) {
  struct val_t a = VALUE(23);
  struct val_t b = VALUE(17);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_lt(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateLt, Interval) {
  struct val_t a = INTERVAL(0, 23);
  struct val_t b = INTERVAL(17, 42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, INTERVAL(17, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, INTERVAL(17, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &B, &A);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, INTERVAL(18, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, INTERVAL(17, 22)))
    .Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateNeg, Value) {
  struct val_t a = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_neg(&X, VALUE(-23)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_neg(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_neg(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_neg(&X, INTERVAL(0, 100)));
  delete(MockProxy);
}

TEST(PropagateNeg, Interval) {
  struct val_t a = INTERVAL(17, 23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(23))).Times(1);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_neg(&X, VALUE(-23)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_neg(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_neg(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_neg(&X, INTERVAL(0, 100)));
  delete(MockProxy);
}

TEST(PropagateAdd, Value) {
  struct val_t a = VALUE(23);
  struct val_t b = VALUE(17);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_add(&X, VALUE(40)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_add(&X, INTERVAL(20, 60)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_add(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_add(&X, INTERVAL(-100, 0)));
  delete(MockProxy);
}

TEST(PropagateAdd, Interval) {
  struct val_t a = INTERVAL(23,24);
  struct val_t b = INTERVAL(17,18);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(23))).Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, VALUE(17))).Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_add(&X, VALUE(40)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_add(&X, INTERVAL(20, 60)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_add(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_add(&X, INTERVAL(-100, 0)));
  delete(MockProxy);
}

TEST(PropagateMul, Value) {
  struct val_t a = VALUE(3);
  struct val_t b = VALUE(7);
  struct val_t z = VALUE(0);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t Z = { .type = CONSTR_TERM, .constr = { .term = &z } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(21)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(1, 30)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_mul(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_mul(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &Z);
  EXPECT_CALL(*MockProxy, update_expr(&X, NULL, &Z))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_mul(&X, INTERVAL(-100, -1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &Z, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &Z, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_mul(&X, INTERVAL(1, 100)));
  delete(MockProxy);
}

TEST(PropagateMul, Interval) {
  struct val_t a = INTERVAL(1,3);
  struct val_t b = INTERVAL(7,8);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(21)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(14, 24)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(-100, 0)));
  delete(MockProxy);
}

TEST(PropagateNot, Value) {
  struct val_t a = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_not(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_not(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_not(&X, INTERVAL(0, 1)));
  delete(MockProxy);
}

TEST(PropagateNot, Interval) {
  struct val_t a = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(0))).Times(1);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_not(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_not(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(1))).Times(1);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_not(&X, VALUE(0)));
  delete(MockProxy);
}

TEST(PropagateAnd, Value) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &B, &A);
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_and(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, NULL, &B))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_and(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateAnd, Interval) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_and(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(1))).Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, VALUE(1))).Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_and(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateOr, Value) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &B, &A);
  EXPECT_CALL(*MockProxy, update_expr(&X, &B, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_or(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, NULL))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_or(&X, VALUE(0)));
  delete(MockProxy);
}

TEST(PropagateOr, Interval) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate_or(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(0))).Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, VALUE(0))).Times(1);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate_or(&X, VALUE(0)));
  delete(MockProxy);
}

TEST(Propagate, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, update_unary_expr(&X, &A))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);
}

TEST(Propagate, Errors) {
  struct val_t c = INTERVAL(0, 1);

  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_CONSTRAINT_TYPE, testing::_)).Times(1);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);
}

TEST(Propagate, Loop) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return(&X));
  EXPECT_EQ(&X, propagate(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, update_expr(&X, &A, &B))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, propagate(&X, c));
  delete(MockProxy);
}

} // end namespace
