#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace update_expr {
#include "../src/util.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
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
  MOCK_METHOD0(eval_cache_invalidate, void(void));
  MOCK_METHOD2(print_error, void (const char *, va_list));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

TEST(UpdateExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  struct constr_t *P, *Q, *R;

  alloc_init(1024);

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);

  EXPECT_EQ(&X, update_expr(&X, &A, &B));
  EXPECT_EQ(X, CONSTRAINT_EXPR(OP_EQ, &A, &B));

  P = update_expr(&X, &C, &B);
  EXPECT_NE(P, &X);
  EXPECT_EQ(*P, CONSTRAINT_EXPR(OP_EQ, &C, &B));

  Q = update_expr(P, &C, &C);
  EXPECT_NE(Q, P);
  EXPECT_EQ(*Q, CONSTRAINT_EXPR(OP_EQ, &C, &C));

  R = update_expr(Q, &A, &B);
  EXPECT_NE(R, Q);
  EXPECT_EQ(*R, CONSTRAINT_EXPR(OP_EQ, &A, &B));
}

TEST(UpdateExpr, Null) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  alloc_init(1024);

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(NULL, update_expr(&X, &A, NULL));
  EXPECT_EQ(NULL, update_expr(&X, NULL, &B));
  EXPECT_EQ(NULL, update_expr(&X, NULL, NULL));
}

TEST(UpdateUnaryExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  struct constr_t *P, *Q;

  alloc_init(1024);

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);

  EXPECT_EQ(&X, update_unary_expr(&X, &A));
  EXPECT_EQ(X, CONSTRAINT_EXPR(OP_EQ, &A, NULL));

  P = update_unary_expr(&X, &B);
  EXPECT_NE(P, &X);
  EXPECT_EQ(*P, CONSTRAINT_EXPR(OP_EQ, &B, NULL));

  Q = update_unary_expr(P, &C);
  EXPECT_NE(Q, P);
  EXPECT_EQ(*Q, CONSTRAINT_EXPR(OP_EQ, &C, NULL));
}

TEST(UpdateUnaryExpr, Null) {
  struct val_t a = VALUE(17);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t X;

  alloc_init(1024);

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);
  EXPECT_EQ(NULL, update_unary_expr(&X, NULL));
}

} // end namespace
