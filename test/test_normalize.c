#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace normalize {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/normalize.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

bool operator==(const struct constr_t& lhs, const struct constr_t& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  }
  if (lhs.type == &CONSTR_TERM) {
    if (lhs.constr.term != rhs.constr.term) {
      return false;
    }
  } else if (lhs.type == &CONSTR_WAND) {
    if (lhs.constr.wand.length != rhs.constr.wand.length ||
        lhs.constr.wand.elems != rhs.constr.wand.elems) {
      return false;
    }
  } else {
    if (lhs.constr.expr.l != rhs.constr.expr.l ||
        lhs.constr.expr.r != rhs.constr.expr.r) {
      return false;
    }
  }
  return true;
}

bool operator==(const struct wand_expr_t& lhs, const struct wand_expr_t& rhs) {
  return lhs.constr == rhs.constr && lhs.prop_tag == rhs.prop_tag;
}

class Mock {
 public:
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD2(patch, size_t(struct wand_expr_t *, const struct wand_expr_t));
  MOCK_METHOD1(print_fatal, void (const char *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, const struct val_t(const struct constr_t *)); \
  MOCK_METHOD2(propagate_ ## NAME, prop_result_t(const struct constr_t *, const struct val_t));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

void *alloc(size_t size) {
  return MockProxy->alloc(size);
}

size_t patch(struct wand_expr_t *loc, const struct wand_expr_t val) {
  return MockProxy->patch(loc, val);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
const struct val_t eval_ ## NAME(const struct constr_t *constr) {       \
  return MockProxy->eval_ ## NAME(constr);                              \
}                                                                       \
prop_result_t propagate_ ## NAME(const struct constr_t *constr, struct val_t val) { \
  return MockProxy->propagate_ ## NAME(constr, val);                    \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

TEST(UpdateExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  struct constr_t P, Q, R;

  X = CONSTRAINT_EXPR(EQ, &A, &B);

  MockProxy = new Mock();
  EXPECT_EQ(&X, update_expr(&X, &A, &B));
  EXPECT_EQ(X, CONSTRAINT_EXPR(EQ, &A, &B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&P));
  EXPECT_EQ(&P, update_expr(&X, &C, &B));
  EXPECT_EQ(P, CONSTRAINT_EXPR(EQ, &C, &B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Q));
  EXPECT_EQ(&Q, update_expr(&P, &C, &C));
  EXPECT_EQ(Q, CONSTRAINT_EXPR(EQ, &C, &C));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&R));
  EXPECT_EQ(&R, update_expr(&Q, &A, &B));
  EXPECT_EQ(R, CONSTRAINT_EXPR(EQ, &A, &B));
  delete(MockProxy);
}

TEST(UpdateUnaryExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  struct constr_t P, Q;

  X = CONSTRAINT_EXPR(EQ, &A, NULL);

  MockProxy = new Mock();
  EXPECT_EQ(&X, update_unary_expr(&X, &A));
  EXPECT_EQ(X, CONSTRAINT_EXPR(EQ, &A, NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&P));
  EXPECT_EQ(&P, update_unary_expr(&X, &B));
  EXPECT_EQ(P, CONSTRAINT_EXPR(EQ, &B, NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Q));
  EXPECT_EQ(&Q, update_unary_expr(&P, &C));
  EXPECT_EQ(Q, CONSTRAINT_EXPR(EQ, &C, NULL));
  delete(MockProxy);
}

TEST(NormalizeEval, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);
  struct val_t c;

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X;
  struct constr_t Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval_eq(&X))
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
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval_eq(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_eval(&X));
  delete(MockProxy);
}

TEST(NormalizeEq, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_CALL(*MockProxy, eval_eq(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_eq(&X));
  delete(MockProxy);
}

TEST(NormalizeLt, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_CALL(*MockProxy, eval_lt(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_lt(&X));
  delete(MockProxy);
}

TEST(NormalizeLt, Neg) {
  struct val_t a = INTERVAL(1, 2);
  struct val_t b = INTERVAL(2, 3);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  Y = CONSTRAINT_EXPR(NEG, &B, NULL);
  Z = CONSTRAINT_EXPR(LT, &X, &Y);
  EXPECT_CALL(*MockProxy, eval_lt(&Z))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_neg(&X))
    .Times(1)
    .WillOnce(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval_neg(&Y))
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

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y, Z, W;

  X = CONSTRAINT_EXPR(NEG, &B, NULL);
  Y = CONSTRAINT_EXPR(LT, &A, &X);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_lt(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_neg(&X))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, eval_neg(&Z))
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

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y, Z, W;

  X = CONSTRAINT_EXPR(NEG, &B, NULL);
  Y = CONSTRAINT_EXPR(LT, &X, &A);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_lt(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_neg(&X))
    .Times(1)
    .WillOnce(::testing::Return(b));
  EXPECT_CALL(*MockProxy, eval_neg(&Z))
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

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, eval_neg(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(-18,-17)));
  EXPECT_EQ(&X, normal_neg(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  Y = CONSTRAINT_EXPR(NEG, &X, NULL);
  EXPECT_CALL(*MockProxy, eval_neg(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(17,18)));
  EXPECT_CALL(*MockProxy, eval_neg(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(INTERVAL(-18,-17)));
  EXPECT_EQ(&A, normal_neg(&Y));
  delete(MockProxy);
}

TEST(NormalizeAdd, Basic) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(0);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_CALL(*MockProxy, eval_add(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(23, 40)));
  EXPECT_EQ(&X, normal_add(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &B, &A);
  EXPECT_CALL(*MockProxy, eval_add(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(23, 40)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_add(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &C);
  EXPECT_CALL(*MockProxy, eval_add(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 17)));
  EXPECT_EQ(&A, normal_add(&X));
  delete(MockProxy);
}

TEST(NormalizeAdd, Pattern) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = INTERVAL(23, 42);
  struct val_t c = VALUE(1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &C);
  Y = CONSTRAINT_EXPR(ADD, &B, &X);
  EXPECT_CALL(*MockProxy, eval_add(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(24, 41)));
  EXPECT_CALL(*MockProxy, eval_add(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(1, 18)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_add(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &C);
  Y = CONSTRAINT_EXPR(ADD, &X, &B);
  EXPECT_CALL(*MockProxy, eval_add(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(24, 41)));
  EXPECT_CALL(*MockProxy, eval_add(&X))
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

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_CALL(*MockProxy, eval_mul(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 17*23)));
  EXPECT_EQ(&X, normal_mul(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &B, &A);
  EXPECT_CALL(*MockProxy, eval_mul(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 17*23)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(1)
    .WillOnce(::testing::Return(&Y));
  EXPECT_EQ(&Y, normal_mul(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &C);
  EXPECT_CALL(*MockProxy, eval_mul(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 17)));
  EXPECT_EQ(&A, normal_mul(&X));
  delete(MockProxy);
}

TEST(NormalizeMul, Pattern) {
  struct val_t a = INTERVAL(0, 17);
  struct val_t b = INTERVAL(23, 42);
  struct val_t c = VALUE(2);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X, Y, Z, W;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &C);
  Y = CONSTRAINT_EXPR(MUL, &B, &X);
  EXPECT_CALL(*MockProxy, eval_mul(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 2*17*42)));
  EXPECT_CALL(*MockProxy, eval_mul(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 2*17)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_mul(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &C);
  Y = CONSTRAINT_EXPR(MUL, &X, &B);
  EXPECT_CALL(*MockProxy, eval_mul(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 2*17*42)));
  EXPECT_CALL(*MockProxy, eval_mul(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 2*17)));
  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&Z))
    .WillOnce(::testing::Return(&W));
  EXPECT_EQ(&W, normal_mul(&Y));
  delete(MockProxy);
}

TEST(NormalizeNot, Basic) {
  struct val_t a = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_not(&X));
  delete(MockProxy);
}

TEST(NormalizeNot, NotNot) {
  struct val_t a = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(NOT, &X, NULL);
  EXPECT_CALL(*MockProxy, eval_not(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&A, normal_not(&Y));
  delete(MockProxy);
}

TEST(NormalizeAnd, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(0);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &A);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&A, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&B, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &B, &A);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&B, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &C);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&C, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &C, &A);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&C, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &B, &C);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &C, &B);
  EXPECT_CALL(*MockProxy, eval_and(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_and(&X));
  delete(MockProxy);
}

TEST(NormalizeAnd, DeMorgan) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y, Z, V, W;

  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(NOT, &B, NULL);
  Z = CONSTRAINT_EXPR(AND, &X, &Y);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_and(&Z))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval_not(&Y))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(b));

  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&W))
    .WillOnce(::testing::Return(&V));
  EXPECT_EQ(&V, normal_and(&Z));
  EXPECT_EQ(&CONSTR_OR, W.type);
  EXPECT_EQ(&A, W.constr.expr.l);
  EXPECT_EQ(&B, W.constr.expr.r);
  delete(MockProxy);
}

TEST(NormalizeAnd, HalfNot) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(AND, &X, &B);
  EXPECT_CALL(*MockProxy, eval_and(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_and(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(AND, &B, &X);
  EXPECT_CALL(*MockProxy, eval_and(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_and(&Y));
  delete(MockProxy);
}

TEST(NormalizeOr, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &A);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&A, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(&B, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &B, &A);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&B, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &C);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&C, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &C, &A);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&C, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &B, &C);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &C, &B);
  EXPECT_CALL(*MockProxy, eval_or(&X))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_EQ(&X, normal_or(&X));
  delete(MockProxy);
}

TEST(NormalizeOr, DeMorgan) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y, Z, V, W;

  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(NOT, &B, NULL);
  Z = CONSTRAINT_EXPR(OR, &X, &Y);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_or(&Z))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_CALL(*MockProxy, eval_not(&Y))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(b));

  EXPECT_CALL(*MockProxy, alloc(sizeof(struct constr_t)))
    .Times(2)
    .WillOnce(::testing::Return(&W))
    .WillOnce(::testing::Return(&V));
  EXPECT_EQ(&V, normal_or(&Z));
  EXPECT_EQ(&CONSTR_AND, W.type);
  EXPECT_EQ(&A, W.constr.expr.l);
  EXPECT_EQ(&B, W.constr.expr.r);
  delete(MockProxy);
}

TEST(NormalizeOr, HalfNot) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OR, &X, &B);
  EXPECT_CALL(*MockProxy, eval_or(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_or(&Y));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  Y = CONSTRAINT_EXPR(OR, &B, &X);
  EXPECT_CALL(*MockProxy, eval_or(&Y))
    .Times(1)
    .WillOnce(::testing::Return(INTERVAL(0, 1)));
  EXPECT_CALL(*MockProxy, eval_not(&X))
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(&Y, normal_or(&Y));
  delete(MockProxy);
}

TEST(NormalizeWand, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct wand_expr_t E [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(&X, normal_wand(&X));
  delete(MockProxy);
}

TEST(NormalizeWand, Patch) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);
  struct val_t x;

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X = CONSTRAINT_EXPR(OR, &A, &B);
  struct wand_expr_t E [5] = { { .constr = &A, .prop_tag = 0 },
                               { .constr = &X, .prop_tag = 0 },
                               { .constr = &B, .prop_tag = 0 },
                               { .constr = &C, .prop_tag = 0 },
                               { .constr = &X, .prop_tag = 0 } };
  struct constr_t Y = CONSTRAINT_WAND(5, E);
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
  EXPECT_CALL(*MockProxy, eval_or(&X))
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

TEST(Normalize, Loop) {
  struct val_t a = INTERVAL(17,23);
  struct val_t c;

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t X, Y;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &A);
  EXPECT_CALL(*MockProxy, eval_eq(&X))
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
