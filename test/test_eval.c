#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace eval {
#include "../src/arith.c"
#include "../src/eval.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

TEST(EvalNeg, Basic) {
  EXPECT_EQ(VALUE(100), eval_neg(VALUE(-100)));
  EXPECT_EQ(VALUE(-100), eval_neg(VALUE(100)));
  EXPECT_EQ(INTERVAL(-100, 1), eval_neg(INTERVAL(-1, 100)));
}

TEST(EvalNeg, Limits) {
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_neg(VALUE(DOMAIN_MIN)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_neg(VALUE(DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_neg(INTERVAL(-1, DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_neg(INTERVAL(DOMAIN_MIN, -1)));
}

TEST(EvalAdd, Basic) {
  EXPECT_EQ(VALUE(100), eval_add(VALUE(1), VALUE(99)));
  EXPECT_EQ(INTERVAL(1,100), eval_add(INTERVAL(0,99), VALUE(1)));
  EXPECT_EQ(INTERVAL(1,100), eval_add(VALUE(1), INTERVAL(0,99)));
  EXPECT_EQ(INTERVAL(-100,200), eval_add(INTERVAL(0, 200), INTERVAL(-100, 0)));
}

TEST(EvalAdd, Max) {
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(VALUE(1), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(VALUE(DOMAIN_MAX), VALUE(1)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(VALUE(-1), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(VALUE(DOMAIN_MAX), VALUE(-1)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(VALUE(1), INTERVAL(0, DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(INTERVAL(0, DOMAIN_MAX), VALUE(1)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(INTERVAL(1, 100), INTERVAL(0, DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(INTERVAL(0, DOMAIN_MAX), INTERVAL(1, 100)));
}

TEST(EvalAdd, Min) {
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(VALUE(1), VALUE(DOMAIN_MIN)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(VALUE(DOMAIN_MIN), VALUE(1)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(VALUE(-1), VALUE(DOMAIN_MIN)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(VALUE(DOMAIN_MIN), VALUE(-1)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(VALUE(1), INTERVAL(DOMAIN_MIN, 0)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(INTERVAL(DOMAIN_MIN, 0), VALUE(1)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(INTERVAL(-100, 1), INTERVAL(DOMAIN_MIN, 0)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(INTERVAL(DOMAIN_MIN, 0), INTERVAL(-100, 1)));
}

TEST(EvalMul, Basic) {
  EXPECT_EQ(VALUE(200), eval_mul(VALUE(2), VALUE(100)));
  EXPECT_EQ(INTERVAL(-2,200), eval_mul(INTERVAL(-1,100), VALUE(2)));
  EXPECT_EQ(INTERVAL(-2,200), eval_mul(VALUE(2), INTERVAL(-1,100)));
  EXPECT_EQ(INTERVAL(-3,3), eval_mul(INTERVAL(2, 3), INTERVAL(-1, 1)));
}

TEST(EvalMul, Max) {
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(VALUE(2), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(VALUE(DOMAIN_MAX), VALUE(2)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(VALUE(-2), VALUE(DOMAIN_MIN)));
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(VALUE(DOMAIN_MIN), VALUE(-2)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(VALUE(1), INTERVAL(1, DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(INTERVAL(1, DOMAIN_MAX), VALUE(1)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(INTERVAL(1, 100), INTERVAL(1, DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(INTERVAL(1, DOMAIN_MAX), INTERVAL(1, 100)));
}

TEST(EvalMul, Min) {
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(VALUE(2), VALUE(DOMAIN_MIN)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(VALUE(DOMAIN_MIN), VALUE(2)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(VALUE(-2), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(VALUE(DOMAIN_MAX), VALUE(-2)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_mul(VALUE(1), INTERVAL(DOMAIN_MIN, 1)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_mul(INTERVAL(DOMAIN_MIN, 1), VALUE(1)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), eval_mul(INTERVAL(-100, 1), INTERVAL(DOMAIN_MIN, 1)));
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), eval_mul(INTERVAL(DOMAIN_MIN, 1), INTERVAL(-100, 1)));
}

TEST(EvalLt, Basic) {
  EXPECT_EQ(VALUE(1), eval_lt(VALUE(-3), VALUE(2)));
  EXPECT_EQ(VALUE(0), eval_lt(VALUE(2), VALUE(-3)));
  EXPECT_EQ(VALUE(1), eval_lt(INTERVAL(-3, 1), VALUE(2)));
  EXPECT_EQ(VALUE(0), eval_lt(VALUE(2), INTERVAL(-3, 1)));
  EXPECT_EQ(VALUE(1), eval_lt(INTERVAL(-3, 1), INTERVAL(2, 4)));
  EXPECT_EQ(VALUE(0), eval_lt(INTERVAL(2, 4), INTERVAL(-3, 1)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(INTERVAL(-3, 4), INTERVAL(1, 2)));
}

TEST(EvalLt, MinMax) {
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(VALUE(DOMAIN_MIN), VALUE(0)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(VALUE(DOMAIN_MAX), VALUE(0)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(VALUE(0), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(VALUE(0), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(INTERVAL(1, DOMAIN_MAX), INTERVAL(DOMAIN_MIN, -1)));
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), VALUE(0)));
}

TEST(EvalEq, Basic) {
  EXPECT_EQ(VALUE(1), eval_eq(VALUE(2), VALUE(2)));
  EXPECT_EQ(VALUE(0), eval_eq(VALUE(2), VALUE(-3)));
  EXPECT_EQ(VALUE(0), eval_eq(INTERVAL(-3, 1), VALUE(2)));
  EXPECT_EQ(VALUE(0), eval_eq(VALUE(2), INTERVAL(-3, 1)));
  EXPECT_EQ(VALUE(0), eval_eq(INTERVAL(-3, 1), INTERVAL(2, 4)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(VALUE(2), INTERVAL(-3, 4)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(INTERVAL(1, 4), INTERVAL(-3, 2)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(INTERVAL(7, 8), INTERVAL(7, 8)));
}

TEST(EvalEq, MinMax) {
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(VALUE(DOMAIN_MIN), VALUE(0)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(VALUE(DOMAIN_MAX), VALUE(0)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(VALUE(0), VALUE(DOMAIN_MIN)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(VALUE(0), VALUE(DOMAIN_MAX)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(INTERVAL(1, DOMAIN_MAX), INTERVAL(DOMAIN_MIN, -1)));
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), VALUE(0)));
}

TEST(EvalNot, Basic) {
  EXPECT_EQ(VALUE(1), eval_not(VALUE(0)));
  EXPECT_EQ(VALUE(0), eval_not(VALUE(1)));
  EXPECT_EQ(INTERVAL(0, 1), eval_not(INTERVAL(0, 1)));
}

TEST(EvalAnd, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_AND, &A, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &B, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &B, &B);
  EXPECT_EQ(VALUE(1), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &A, &C);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &C, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &B, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &C, &B);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
  X = CONSTRAINT_EXPR(OP_AND, &C, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
}

TEST(EvalOr, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_OR, &A, &A);
  EXPECT_EQ(VALUE(0), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &B, &A);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &B, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &A, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &C, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &B, &C);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &C, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OP_OR, &C, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
}

TEST(EvalWand, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };

  struct constr_t *E1 [2] = { &A, &A };
  struct constr_t X1 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E1 } } };
  EXPECT_EQ(VALUE(0), eval_wand(&X1));

  struct constr_t *E2 [2] = { &A, &B };
  struct constr_t X2 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E2 } } };
  EXPECT_EQ(VALUE(0), eval_wand(&X2));

  struct constr_t *E3 [2] = { &B, &A };
  struct constr_t X3 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E3 } } };
  EXPECT_EQ(VALUE(0), eval_wand(&X3));

  struct constr_t *E4 [2] = { &B, &B };
  struct constr_t X4 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E4 } } };
  EXPECT_EQ(VALUE(1), eval_wand(&X4));

  struct constr_t *E5 [2] = { &A, &C };
  struct constr_t X5 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E5 } } };
  EXPECT_EQ(VALUE(0), eval_wand(&X5));

  struct constr_t *E6 [2] = { &C, &A };
  struct constr_t X6 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E6 } } };
  EXPECT_EQ(VALUE(0), eval_wand(&X6));

  struct constr_t *E7 [2] = { &B, &C };
  struct constr_t X7 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E7 } } };
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X7));

  struct constr_t *E8 [2] = { &C, &B };
  struct constr_t X8 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E8 } } };
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X8));

  struct constr_t *E9 [2] = { &C, &C };
  struct constr_t X9 = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E9 } } };
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X9));
}

TEST(Eval, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &A);
  EXPECT_EQ(VALUE(1), eval(&X));
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_LT, &A, &A);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(VALUE(1), eval(&X));
  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_NEG, &B, NULL);
  EXPECT_EQ(VALUE(-1), eval(&X));
  X = CONSTRAINT_EXPR(OP_ADD, &A, &A);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_ADD, &B, &B);
  EXPECT_EQ(VALUE(2), eval(&X));
  X = CONSTRAINT_EXPR(OP_MUL, &A, &A);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_MUL, &B, &B);
  EXPECT_EQ(VALUE(1), eval(&X));
  X = CONSTRAINT_EXPR(OP_NOT, &A, NULL);
  EXPECT_EQ(VALUE(1), eval(&X));
  X = CONSTRAINT_EXPR(OP_NOT, &B, NULL);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_AND, &B, &B);
  EXPECT_EQ(VALUE(1), eval(&X));
  X = CONSTRAINT_EXPR(OP_OR, &A, &A);
  EXPECT_EQ(VALUE(0), eval(&X));
  X = CONSTRAINT_EXPR(OP_OR, &A, &B);
  EXPECT_EQ(VALUE(1), eval(&X));
}

TEST(Eval, Wand) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t *E [2] = { &A, &B };
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E } } };

  MockProxy = new Mock();
  EXPECT_EQ(VALUE(0), eval(&X));
  delete(MockProxy);
}

TEST(Eval, Cache) {
  struct val_t a = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &A);
  EXPECT_EQ(VALUE(1), eval(&X));
  EXPECT_EQ(VALUE(1), eval(&X));
  EXPECT_EQ(VALUE(1), eval(&X));
}

TEST(Eval, Errors) {
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  eval(&X);
  delete(MockProxy);
}

TEST(EvalCacheInval, Basic) {
  _eval_cache_tag = 77;
  eval_cache_invalidate();
  EXPECT_EQ(78, _eval_cache_tag);
}

} // end namespace
