#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace update_expr {
#include "../src/csolve.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}
bool operator==(const struct constr_t& lhs, const struct constr_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.constr, &rhs.constr, sizeof(lhs.constr)) == 0;
}

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

TEST(UpdateExpr, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = VALUE(42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t X;

  struct constr_t *P, *Q, *R;

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

  X = CONSTRAINT_EXPR(OP_EQ, &A, NULL);
  EXPECT_EQ(NULL, update_unary_expr(&X, NULL));
}

} // end namespace
