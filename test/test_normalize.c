#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace normalize {
#include "../src/arith.c"
#include "../src/normalize.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

bool operator==(const struct constr_t& lhs, const struct constr_t& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  }
  if (lhs.type == CONSTR_TERM) {
    if (lhs.constr.term != rhs.constr.term) {
      return false;
    }
  }
  if (lhs.type == CONSTR_EXPR) {
    if (lhs.constr.expr.op != rhs.constr.expr.op ||
        lhs.constr.expr.l != rhs.constr.expr.l ||
        lhs.constr.expr.r != rhs.constr.expr.r) {
      return false;
    }
  }
  return true;
}

bool operator==(const struct wand_expr_t& lhs, const struct wand_expr_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(eval, const struct val_t(const struct constr_t *));
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD2(patch, size_t(struct wand_expr_t *, const struct wand_expr_t));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

const struct val_t eval(const struct constr_t *constr) {
  return MockProxy->eval(constr);
}

void *alloc(size_t size) {
  return MockProxy->alloc(size);
}

size_t patch(struct wand_expr_t *loc, const struct wand_expr_t val) {
  return MockProxy->patch(loc, val);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

TEST(UpdateExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  struct constr_t P, Q, R;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);

  MockProxy = new Mock();
  EXPECT_EQ(&X, update_expr(&X, &A, &B));
  EXPECT_EQ(X, CONSTRAINT_EXPR(OP_EQ, &A, &B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&P));
  EXPECT_EQ(&P, update_expr(&X, &C, &B));
  EXPECT_EQ(P, CONSTRAINT_EXPR(OP_EQ, &C, &B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Q));
  EXPECT_EQ(&Q, update_expr(&P, &C, &C));
  EXPECT_EQ(Q, CONSTRAINT_EXPR(OP_EQ, &C, &C));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&R));
  EXPECT_EQ(&R, update_expr(&Q, &A, &B));
  EXPECT_EQ(R, CONSTRAINT_EXPR(OP_EQ, &A, &B));
  delete(MockProxy);
}

TEST(UpdateUnaryExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  struct constr_t P, Q;

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);

  MockProxy = new Mock();
  EXPECT_EQ(&X, update_unary_expr(&X, &A));
  EXPECT_EQ(X, CONSTRAINT_EXPR(OP_EQ, &A, NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&P));
  EXPECT_EQ(&P, update_unary_expr(&X, &B));
  EXPECT_EQ(P, CONSTRAINT_EXPR(OP_EQ, &B, NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Q));
  EXPECT_EQ(&Q, update_unary_expr(&P, &C));
  EXPECT_EQ(Q, CONSTRAINT_EXPR(OP_EQ, &C, NULL));
  delete(MockProxy);
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
  EXPECT_EQ(&X, normal_lt(&X));
  delete(MockProxy);
}

TEST(NormalizeLt, Neg) {
  struct val_t a = INTERVAL(1, 2);
  struct val_t b = INTERVAL(2, 3);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NEG, &B, NULL);
  Z = CONSTRAINT_EXPR(OP_LT, &X, &Y);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval(&Y))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_lt(&Z));
  delete(MockProxy);
}

TEST(NormalizeLt, LeftConstNeg) {
  struct val_t a = VALUE(1);
  struct val_t b = INTERVAL(2, 3);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y, Z, W;

  X = CONSTRAINT_EXPR(OP_NEG, &B, NULL);
  Y = CONSTRAINT_EXPR(OP_LT, &A, &X);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, eval(&Z))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_lt(&Y));
  delete(MockProxy);
}

TEST(NormalizeLt, RightConstNeg) {
  struct val_t a = VALUE(1);
  struct val_t b = INTERVAL(2, 3);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y, Z, W;

  X = CONSTRAINT_EXPR(OP_NEG, &B, NULL);
  Y = CONSTRAINT_EXPR(OP_LT, &X, &A);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, eval(&Z))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_lt(&Y));
  delete(MockProxy);
}

TEST(NormalizeNeg, Basic) {
  struct val_t a = INTERVAL(17, 18);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(&X, normal_neg(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NEG, &X, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(INTERVAL(-18,-17)));
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
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(&X, normal_add(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &B, &A);
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_add(&X));
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
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &C);
  Y = CONSTRAINT_EXPR(OP_ADD, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_add(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &C);
  Y = CONSTRAINT_EXPR(OP_ADD, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_add(&Y));
  delete(MockProxy);
}

TEST(NormalizeMul, Basic) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, normal_mul(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &B, &A);
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_mul(&X));
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
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &C);
  Y = CONSTRAINT_EXPR(OP_MUL, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_mul(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &C);
  Y = CONSTRAINT_EXPR(OP_MUL, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_mul(&Y));
  delete(MockProxy);
}

TEST(NormalizeNot, Basic) {
  struct val_t a = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(&X, normal_not(&X));
  delete(MockProxy);
}

TEST(NormalizeNot, NotNot) {
  struct val_t a = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NOT, &X, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(a));
  EXPECT_EQ(&A, normal_not(&Y));
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
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &C, &B);
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);
}

TEST(NormalizeAnd, DeMorgan) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y, Z, V, W;

  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NOT, &B, NULL);
  Z = CONSTRAINT_EXPR(OP_AND, &X, &Y);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval(&Y))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(b));

  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&W))
    .WillOnce(::testing::Return(&V));
  EXPECT_EQ(&V, normal_and(&Z));
  EXPECT_EQ(CONSTR_EXPR, W.type);
  EXPECT_EQ(OP_OR, W.constr.expr.op);
  EXPECT_EQ(&A, W.constr.expr.l);
  EXPECT_EQ(&B, W.constr.expr.r);
  delete(MockProxy);
}

TEST(NormalizeAnd, HalfNot) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_AND, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_and(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_AND, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_and(&Y));
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
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &C, &B);
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);
}

TEST(NormalizeOr, DeMorgan) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y, Z, V, W;

  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_NOT, &B, NULL);
  Z = CONSTRAINT_EXPR(OP_OR, &X, &Y);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval(&Y))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(b));

  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&W))
    .WillOnce(::testing::Return(&V));
  EXPECT_EQ(&V, normal_or(&Z));
  EXPECT_EQ(CONSTR_EXPR, W.type);
  EXPECT_EQ(OP_AND, W.constr.expr.op);
  EXPECT_EQ(&A, W.constr.expr.l);
  EXPECT_EQ(&B, W.constr.expr.r);
  delete(MockProxy);
}

TEST(NormalizeOr, HalfNot) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_OR, &X, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_or(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OP_OR, &B, &X);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_or(&Y));
  delete(MockProxy);
}

TEST(NormalizeWand, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct wand_expr_t E [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E } } };

  MockProxy = new Mock();
  EXPECT_EQ(&X, normal_wand(&X));
  delete(MockProxy);
}

TEST(NormalizeWand, Patch) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);
  struct val_t x;

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  struct wand_expr_t E [5] = { { .constr = &A, .prop_tag = 0 },
                               { .constr = &X, .prop_tag = 0 },
                               { .constr = &B, .prop_tag = 0 },
                               { .constr = &C, .prop_tag = 0 },
                               { .constr = &X, .prop_tag = 0 } };
  struct constr_t Y = { .type = CONSTR_WAND, .constr = { .wand = { .length = 5, .elems = E } } };
  struct constr_t V, W;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct val_t)))
    .Times(2)
    .WillOnce(::testing::Return(&x))
    .WillOnce(::testing::Return(&x));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&W))
    .WillOnce(::testing::Return(&V));
  EXPECT_CALL(*MockProxy, patch(&Y.constr.wand.elems[1], (struct wand_expr_t){ &W, 0}))
    .Times(1);
  EXPECT_CALL(*MockProxy, patch(&Y.constr.wand.elems[4], (struct wand_expr_t){ &V, 0}))
    .Times(1);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(2)
    .WillOnce(::testing::Return(VALUE(1)))
    .WillOnce(::testing::Return(VALUE(1)));
  EXPECT_EQ(&Y, normal_wand(&Y));
  EXPECT_EQ(5, Y.constr.wand.length);
  EXPECT_EQ(E, Y.constr.wand.elems);
  EXPECT_EQ(&A, Y.constr.wand.elems[0].constr);
  EXPECT_EQ(&X, Y.constr.wand.elems[1].constr);
  EXPECT_EQ(&B, Y.constr.wand.elems[2].constr);
  EXPECT_EQ(&C, Y.constr.wand.elems[3].constr);
  EXPECT_EQ(&X, Y.constr.wand.elems[4].constr);
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
  EXPECT_EQ(&Y, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);
}

TEST(Normalize, Wand) {
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 0, .elems = NULL } } };

  MockProxy = new Mock();
  EXPECT_EQ(&X, normalize_step(&X));
  delete(MockProxy);
 }

TEST(Normalize, Errors) {
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, eval(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(DOMAIN_MIN, DOMAIN_MAX)));
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  normalize_step(&X);
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_EQ(&X, normalize_step(&X));
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
