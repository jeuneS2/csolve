#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace propagate {
#include "../src/arith.c"
#include "../src/eval.c"
#include "../src/propagate.c"

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

class Mock {
 public:
  MOCK_METHOD2(bind, size_t(struct val_t *, const struct val_t));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

size_t bind(struct val_t *loc, const struct val_t val) {
  return MockProxy->bind(loc, val);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

TEST(UpdateExpr, NonNull) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(&X, update_expr(&X, &A, &B));
}

TEST(UpdateExpr, Null) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(NULL, update_expr(&X, &A, NULL));
  EXPECT_EQ(NULL, update_expr(&X, NULL, &B));
  EXPECT_EQ(NULL, update_expr(&X, NULL, NULL));
}

TEST(UpdateUnaryExpr, NonNull) {
  struct val_t a = VALUE(17);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);
  EXPECT_EQ(&X, update_unary_expr(&X, &A));
}

TEST(UpdateUnaryExpr, Null) {
  struct val_t a = VALUE(17);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);
  EXPECT_EQ(NULL, update_unary_expr(&X, NULL));
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
  EXPECT_EQ(&X, propagate_eq(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(&X, propagate_eq(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
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
  EXPECT_EQ(&X, propagate_eq(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(&X, propagate_eq(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, INTERVAL(17, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, INTERVAL(17, 23)))
    .Times(1);
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
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(&X, propagate_lt(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
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
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(&X, propagate_lt(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &B, &A);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, INTERVAL(18, 23)))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(B.constr.term, INTERVAL(17, 22)))
    .Times(1);
  EXPECT_EQ(&X, propagate_lt(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateNeg, Value) {
  struct val_t a = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(&X, propagate_neg(&X, VALUE(-23)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(&X, propagate_neg(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(NULL, propagate_neg(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
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
  EXPECT_EQ(&X, propagate_neg(&X, VALUE(-23)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(&X, propagate_neg(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(NULL, propagate_neg(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
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
  EXPECT_EQ(&X, propagate_add(&X, VALUE(40)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(&X, propagate_add(&X, INTERVAL(20, 60)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(NULL, propagate_add(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
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
  EXPECT_EQ(&X, propagate_add(&X, VALUE(40)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(&X, propagate_add(&X, INTERVAL(20, 60)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(NULL, propagate_add(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
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
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(21)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(NULL, propagate_mul(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(1, 30)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(NULL, propagate_mul(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(NULL, propagate_mul(&X, INTERVAL(-100, 0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &Z);
  EXPECT_EQ(NULL, propagate_mul(&X, INTERVAL(-100, -1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &Z, &B);
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
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(21)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(14, 24)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, propagate_mul(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, propagate_mul(&X, INTERVAL(-100, 0)));
  delete(MockProxy);
}

TEST(PropagateNot, Value) {
  struct val_t a = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(&X, propagate_not(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(NULL, propagate_not(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
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
  EXPECT_EQ(&X, propagate_not(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(&X, propagate_not(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(A.constr.term, VALUE(1))).Times(1);
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
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &B, &A);
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(&X, propagate_and(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(NULL, propagate_and(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(PropagateAnd, Interval) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = VALUE(0);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(&X, propagate_and(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(&X, propagate_and(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
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
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &B, &A);
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(&X, propagate_or(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(NULL, propagate_or(&X, VALUE(0)));
  delete(MockProxy);
}

TEST(PropagateOr, Interval) {
  struct val_t a = INTERVAL(0, 1);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(&X, propagate_or(&X, VALUE(1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(&X, propagate_or(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(NULL, propagate_or(&X, VALUE(0)));
  delete(MockProxy);
}

TEST(PropagateWand, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t *E [2] = { &A, &B };
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E } } };

  MockProxy = new Mock();
  EXPECT_EQ(&X, propagate_wand(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(NULL, propagate_wand(&X, VALUE(1)));
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
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_ADD, &A, &B);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_MUL, &A, &B);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(&X, prop(&X, c));
  delete(MockProxy);
}

TEST(Propagate, Wand) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t *E [2] = { &A, &B };
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E } } };

  MockProxy = new Mock();
  EXPECT_EQ(&X, prop(&X, VALUE(0)));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(NULL, prop(&X, VALUE(1)));
  delete(MockProxy);
}

TEST(Propagate, Errors) {
  struct val_t c = INTERVAL(0, 1);

  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  prop(&X, c);
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE)).Times(1);
  prop(&X, c);
  delete(MockProxy);
}

TEST(Propagate, Loop) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(&X, propagate(&X, INTERVAL(0, 1)));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(NULL, propagate(&X, VALUE(1)));
  delete(MockProxy);
}

} // end namespace
