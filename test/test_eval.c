#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace eval {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/eval.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(print_fatal, void (const char *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD2(propagate_ ## NAME, prop_result_t(const struct constr_t *, const struct val_t)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
prop_result_t propagate_ ## NAME(const struct constr_t *constr, struct val_t val) { \
  return MockProxy->propagate_ ## NAME(constr, val);                    \
}                                                                       \
struct constr_t *normal_ ## NAME(struct constr_t *constr) {             \
  return MockProxy->normal_ ## NAME(constr);                            \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

TEST(EvalNeg, Basic) {
  struct val_t a = VALUE(-100);
  struct val_t b = VALUE(100);
  struct val_t c = INTERVAL(-1, 100);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(VALUE(100), eval_neg(&X));
  X = CONSTRAINT_EXPR(NEG, &B, NULL);
  EXPECT_EQ(VALUE(-100), eval_neg(&X));
  X = CONSTRAINT_EXPR(NEG, &C, NULL);
  EXPECT_EQ(INTERVAL(-100, 1), eval_neg(&X));
}

TEST(EvalNeg, Limits) {
  struct val_t a = VALUE(DOMAIN_MIN);
  struct val_t b = VALUE(DOMAIN_MAX);
  struct val_t c = INTERVAL(-1, DOMAIN_MAX);
  struct val_t d = INTERVAL(DOMAIN_MIN, -1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t X;

  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_neg(&X));
  X = CONSTRAINT_EXPR(NEG, &B, NULL);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_neg(&X));
  X = CONSTRAINT_EXPR(NEG, &C, NULL);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_neg(&X));
  X = CONSTRAINT_EXPR(NEG, &D, NULL);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_neg(&X));
}

TEST(EvalAdd, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(99);
  struct val_t c = INTERVAL(0, 99);
  struct val_t d = INTERVAL(0, 200);
  struct val_t e = INTERVAL(-100, 0);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t X;

  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(VALUE(100), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &C, &A);
  EXPECT_EQ(INTERVAL(1,100), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &A, &C);
  EXPECT_EQ(INTERVAL(1,100), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &D, &E);
  EXPECT_EQ(INTERVAL(-100,200), eval_add(&X));
}

TEST(EvalAdd, Max) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(-1);
  struct val_t c = VALUE(DOMAIN_MAX);
  struct val_t d = INTERVAL(1, 100);
  struct val_t e = INTERVAL(0, DOMAIN_MAX);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t X;

  X = CONSTRAINT_EXPR(ADD, &A, &C);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &C, &A);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &B, &C);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &C, &B);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &A, &E);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &E, &A);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &D, &E);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &E, &D);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_add(&X));
}

TEST(EvalAdd, Min) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(-1);
  struct val_t c = VALUE(DOMAIN_MIN);
  struct val_t d = INTERVAL(-100, 1);
  struct val_t e = INTERVAL(DOMAIN_MIN, 0);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t X;

  X = CONSTRAINT_EXPR(ADD, &A, &C);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &C, &A);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &B, &C);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &C, &B);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &A, &E);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &E, &A);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &D, &E);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(&X));
  X = CONSTRAINT_EXPR(ADD, &E, &D);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_add(&X));
}

TEST(EvalMul, Basic) {
  struct val_t a = VALUE(2);
  struct val_t b = VALUE(100);
  struct val_t c = INTERVAL(-1,100);
  struct val_t d = INTERVAL(2, 3);
  struct val_t e = INTERVAL(-1, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t X;

  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(VALUE(200), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &C, &A);
  EXPECT_EQ(INTERVAL(-2,200), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &A, &C);
  EXPECT_EQ(INTERVAL(-2,200), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &D, &E);
  EXPECT_EQ(INTERVAL(-3,3), eval_mul(&X));
}

TEST(EvalMul, Max) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);
  struct val_t c = VALUE(-2);
  struct val_t d = VALUE(DOMAIN_MAX);
  struct val_t e = VALUE(DOMAIN_MIN);
  struct val_t f = INTERVAL(1,100);
  struct val_t g = INTERVAL(1,DOMAIN_MAX);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t G = CONSTRAINT_TERM(&g);
  struct constr_t X;

  X = CONSTRAINT_EXPR(MUL, &B, &D);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &D, &B);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &C, &E);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &E, &C);
  EXPECT_EQ(VALUE(DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &A, &G);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &G, &A);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &F, &G);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &G, &F);
  EXPECT_EQ(INTERVAL(1, DOMAIN_MAX), eval_mul(&X));
}

TEST(EvalMul, Min) {
  struct val_t a = VALUE(1);
  struct val_t b = VALUE(2);
  struct val_t c = VALUE(-2);
  struct val_t d = VALUE(DOMAIN_MAX);
  struct val_t e = VALUE(DOMAIN_MIN);
  struct val_t f = INTERVAL(-100,1);
  struct val_t g = INTERVAL(DOMAIN_MIN, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t G = CONSTRAINT_TERM(&g);
  struct constr_t X;

  X = CONSTRAINT_EXPR(MUL, &B, &E);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &E, &B);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &C, &D);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &D, &C);
  EXPECT_EQ(VALUE(DOMAIN_MIN), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &A, &G);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &G, &A);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, 1), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &F, &G);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), eval_mul(&X));
  X = CONSTRAINT_EXPR(MUL, &G, &F);
  EXPECT_EQ(INTERVAL(DOMAIN_MIN, DOMAIN_MAX), eval_mul(&X));
}

TEST(EvalLt, Basic) {
  struct val_t a = VALUE(2);
  struct val_t b = VALUE(-3);
  struct val_t c = INTERVAL(-3, 1);
  struct val_t d = INTERVAL(2, 4);
  struct val_t e = INTERVAL(-3, 4);
  struct val_t f = INTERVAL(1, 2);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t X;

  X = CONSTRAINT_EXPR(LT, &B, &A);
  EXPECT_EQ(VALUE(1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(VALUE(0), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &C, &A);
  EXPECT_EQ(VALUE(1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &A, &C);
  EXPECT_EQ(VALUE(0), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &C, &D);
  EXPECT_EQ(VALUE(1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &D, &C);
  EXPECT_EQ(VALUE(0), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &E, &F);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
}

TEST(EvalLt, MinMax) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(DOMAIN_MAX);
  struct val_t c = VALUE(DOMAIN_MIN);
  struct val_t d = INTERVAL(1,DOMAIN_MAX);
  struct val_t e = INTERVAL(DOMAIN_MIN,-1);
  struct val_t f = INTERVAL(DOMAIN_MIN,DOMAIN_MAX);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t X;

  X = CONSTRAINT_EXPR(LT, &C, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &B, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &A, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &D, &E);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
  X = CONSTRAINT_EXPR(LT, &F, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_lt(&X));
}

TEST(EvalEq, Basic) {
  struct val_t a = VALUE(2);
  struct val_t b = VALUE(-3);
  struct val_t c = INTERVAL(-3, 1);
  struct val_t d = INTERVAL(2, 4);
  struct val_t e = INTERVAL(-3, 4);
  struct val_t f = INTERVAL(1, 4);
  struct val_t g = INTERVAL(-3, 2);
  struct val_t h = INTERVAL(7, 8);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t G = CONSTRAINT_TERM(&g);
  struct constr_t H = CONSTRAINT_TERM(&h);
  struct constr_t X;

  X = CONSTRAINT_EXPR(EQ, &A, &A);
  EXPECT_EQ(VALUE(1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(VALUE(0), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &C, &A);
  EXPECT_EQ(VALUE(0), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &A, &C);
  EXPECT_EQ(VALUE(0), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &C, &D);
  EXPECT_EQ(VALUE(0), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &A, &E);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &F, &G);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &H, &H);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
}

TEST(EvalEq, MinMax) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(DOMAIN_MIN);
  struct val_t c = VALUE(DOMAIN_MAX);
  struct val_t d = INTERVAL(DOMAIN_MIN, -1);
  struct val_t e = INTERVAL(1, DOMAIN_MAX);
  struct val_t f = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t D = CONSTRAINT_TERM(&d);
  struct constr_t E = CONSTRAINT_TERM(&e);
  struct constr_t F = CONSTRAINT_TERM(&f);
  struct constr_t X;

  X = CONSTRAINT_EXPR(EQ, &B, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &C, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &A, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &E, &D);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
  X = CONSTRAINT_EXPR(EQ, &F, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_eq(&X));
}

TEST(EvalNot, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_EQ(VALUE(1), eval_not(&X));
  X = CONSTRAINT_EXPR(NOT, &B, NULL);
  EXPECT_EQ(VALUE(0), eval_not(&X));
  X = CONSTRAINT_EXPR(NOT, &C, NULL);
  EXPECT_EQ(INTERVAL(0, 1), eval_not(&X));
}

TEST(EvalAnd, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  X = CONSTRAINT_EXPR(AND, &A, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &B, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &B, &B);
  EXPECT_EQ(VALUE(1), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &A, &C);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &C, &A);
  EXPECT_EQ(VALUE(0), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &B, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &C, &B);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
  X = CONSTRAINT_EXPR(AND, &C, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_and(&X));
}

TEST(EvalOr, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);
  struct constr_t X;

  X = CONSTRAINT_EXPR(OR, &A, &A);
  EXPECT_EQ(VALUE(0), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &B, &A);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &B, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &A, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &C, &A);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &B, &C);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &C, &B);
  EXPECT_EQ(VALUE(1), eval_or(&X));
  X = CONSTRAINT_EXPR(OR, &C, &C);
  EXPECT_EQ(INTERVAL(0, 1), eval_or(&X));
}

TEST(EvalWand, Basic) {
  struct val_t a = VALUE(0);
  struct val_t b = VALUE(1);
  struct val_t c = INTERVAL(0, 1);

  struct constr_t A = CONSTRAINT_TERM(&a);
  struct constr_t B = CONSTRAINT_TERM(&b);
  struct constr_t C = CONSTRAINT_TERM(&c);

  struct wand_expr_t E1 [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &A, .prop_tag = 0 } };
  struct constr_t X1 = CONSTRAINT_WAND(2, E1);
  EXPECT_EQ(VALUE(0), eval_wand(&X1));

  struct wand_expr_t E2 [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X2 = CONSTRAINT_WAND(2, E2);
  EXPECT_EQ(VALUE(0), eval_wand(&X2));

  struct wand_expr_t E3 [2] = { { .constr = &B, .prop_tag = 0 }, { .constr = &A, .prop_tag = 0 } };
  struct constr_t X3 = CONSTRAINT_WAND(2, E3);
  EXPECT_EQ(VALUE(0), eval_wand(&X3));

  struct wand_expr_t E4 [2] = { { .constr = &B, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X4 = CONSTRAINT_WAND(2, E4);
  EXPECT_EQ(VALUE(1), eval_wand(&X4));

  struct wand_expr_t E5 [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &C, .prop_tag = 0 } };
  struct constr_t X5 = CONSTRAINT_WAND(2, E5);
  EXPECT_EQ(VALUE(0), eval_wand(&X5));

  struct wand_expr_t E6 [2] = { { .constr = &C, .prop_tag = 0 }, { .constr = &A, .prop_tag = 0 } };
  struct constr_t X6 = CONSTRAINT_WAND(2, E6);
  EXPECT_EQ(VALUE(0), eval_wand(&X6));

  struct wand_expr_t E7 [2] = { { .constr = &B, .prop_tag = 0 }, { .constr = &C, .prop_tag = 0 } };
  struct constr_t X7 = CONSTRAINT_WAND(2, E7);
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X7));

  struct wand_expr_t E8 [2] = { { .constr = &C, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X8 = CONSTRAINT_WAND(2, E8);
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X8));

  struct wand_expr_t E9 [2] = { { .constr = &C, .prop_tag = 0 }, { .constr = &C, .prop_tag = 0 } };
  struct constr_t X9 = CONSTRAINT_WAND(2, E9);
  EXPECT_EQ(INTERVAL(0, 1), eval_wand(&X9));
}

} // end namespace
