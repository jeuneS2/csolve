#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace propagate {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/eval.c"
#include "../src/propagate.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

bool operator==(const struct constr_t& lhs, const struct constr_t& rhs) {
  if (lhs.type != rhs.type) {
    return false;
  }
  if (lhs.type == &CONSTR_TERM) {
    if (!(lhs.constr.term.val == rhs.constr.term.val) ||
        lhs.constr.term.env != rhs.constr.term.env) {
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

class Mock {
 public:
  MOCK_METHOD3(bind, void(struct env_t *, const struct val_t, const struct wand_expr_t *));
  MOCK_METHOD0(conflict_reset, void(void));
  MOCK_METHOD2(conflict_create, void(struct env_t *, const struct wand_expr_t *));
  MOCK_METHOD0(strategy_create_conflicts, bool(void));
  MOCK_METHOD1(strategy_var_order_update, void(struct env_t *));
  MOCK_METHOD2(patch, size_t(struct wand_expr_t *, struct constr_t *));
  MOCK_METHOD1(print_fatal, void (const char *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

uint64_t props;

void bind(struct env_t *var, const struct val_t val, const struct wand_expr_t *clause) {
  MockProxy->bind(var, val, clause);
}

void conflict_reset(void) {
  MockProxy->conflict_reset();
}

void conflict_create(struct env_t *var, const struct wand_expr_t *clause) {
  MockProxy->conflict_create(var, clause);
}

bool strategy_create_conflicts(void) {
  return MockProxy->strategy_create_conflicts();
}

void strategy_var_order_update(struct env_t *var) {
  MockProxy->strategy_var_order_update(var);
}

size_t patch(struct wand_expr_t *loc, struct constr_t *constr) {
  return MockProxy->patch(loc, constr);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                \
struct constr_t *normal_ ## NAME(struct constr_t *constr) { \
  return MockProxy->normal_ ## NAME(constr);                \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

TEST(PropagateTerm, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(23));

  MockProxy = new Mock();
  EXPECT_EQ(PROP_NONE, propagate_term(&A, VALUE(23), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, VALUE(17), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, VALUE(42), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_NONE, propagate_term(&A, INTERVAL(17, 42), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, INTERVAL(0, 17), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, INTERVAL(42, 100), NULL));
  delete(MockProxy);
}

TEST(PropagateTerm, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(23, 24));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(23), NULL)).Times(1);
  EXPECT_EQ(1, propagate_term(&A, VALUE(23), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, VALUE(17), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, VALUE(42), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_NONE, propagate_term(&A, INTERVAL(17, 42), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(23), NULL)).Times(1);
  EXPECT_EQ(1, propagate_term(&A, INTERVAL(-1, 23), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, INTERVAL(0, 17), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_term(&A, INTERVAL(42, 100), NULL));
  delete(MockProxy);
}

TEST(PropagateEq, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(17));
  struct constr_t B = CONSTRAINT_TERM(VALUE(23));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_eq(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_eq(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_eq(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateEq, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 23));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(17, 42));
  struct env_t f = { .key = NULL, .val = &B, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  B.constr.term.env = &f;
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_eq(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_eq(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_CALL(*MockProxy, bind(&e, INTERVAL(17, 23), NULL))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(&f, INTERVAL(17, 23), NULL))
    .Times(1);
  EXPECT_EQ(2, propagate_eq(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateLt, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(23));
  struct constr_t B = CONSTRAINT_TERM(VALUE(17));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_lt(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_lt(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_lt(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateLt, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 23));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(17, 42));
  struct env_t f = { .key = NULL, .val = &B, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  B.constr.term.env = &f;
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_CALL(*MockProxy, bind(&e, INTERVAL(17, 23), NULL))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(&f, INTERVAL(17, 23), NULL))
    .Times(1);
  EXPECT_EQ(2, propagate_lt(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_lt(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_lt(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &B, &A);
  EXPECT_CALL(*MockProxy, bind(&e, INTERVAL(18, 23), NULL))
    .Times(1);
  EXPECT_CALL(*MockProxy, bind(&f, INTERVAL(17, 22), NULL))
    .Times(1);
  EXPECT_EQ(2, propagate_lt(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateNeg, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(23));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_neg(&X, VALUE(-23), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_neg(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(PROP_ERROR, propagate_neg(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(PROP_ERROR, propagate_neg(&X, INTERVAL(0, 100), NULL));
  delete(MockProxy);
}

TEST(PropagateNeg, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(17, 23));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(23), NULL)).Times(1);
  EXPECT_EQ(1, propagate_neg(&X, VALUE(-23), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_neg(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_neg(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&e))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_neg(&X, INTERVAL(0, 100), NULL));
  delete(MockProxy);
}

TEST(PropagateAdd, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(23));
  struct constr_t B = CONSTRAINT_TERM(VALUE(17));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_add(&X, VALUE(40), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_add(&X, INTERVAL(20, 60), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_add(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_add(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);
}

TEST(PropagateAdd, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(23,24));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(17,18));
  struct env_t f = { .key = NULL, .val = &B, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  B.constr.term.env = &f;
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(23), NULL)).Times(1);
  EXPECT_CALL(*MockProxy, bind(&f, VALUE(17), NULL)).Times(1);
  EXPECT_EQ(2, propagate_add(&X, VALUE(40), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_add(&X, INTERVAL(20, 60), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&f))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_add(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, strategy_var_order_update(&f))
    .Times(1);
  EXPECT_EQ(PROP_ERROR, propagate_add(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);
}

TEST(PropagateMul, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(3));
  struct constr_t B = CONSTRAINT_TERM(VALUE(7));
  struct constr_t Z = CONSTRAINT_TERM(VALUE(0));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, VALUE(21), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_mul(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, INTERVAL(1, 30), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_mul(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_mul(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &Z);
  EXPECT_EQ(PROP_ERROR, propagate_mul(&X, INTERVAL(-100, -1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &Z, &B);
  EXPECT_EQ(PROP_ERROR, propagate_mul(&X, INTERVAL(1, 100), NULL));
  delete(MockProxy);
}

TEST(PropagateMul, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(1,3));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(7,8));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, VALUE(21), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, INTERVAL(14, 24), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_mul(&X, INTERVAL(-100, 0), NULL));
  delete(MockProxy);
}

TEST(PropagateNot, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_not(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_EQ(PROP_ERROR, propagate_not(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_not(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);
}

TEST(PropagateNot, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct env_t e = { .key = NULL, .val = &A, .binds = NULL,
                     .clauses = { .length = 0, .elems = NULL },
                     .order = 0, .prio = 0, .level = 0 };
  A.constr.term.env = &e;
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(0), NULL)).Times(1);
  EXPECT_EQ(1, propagate_not(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_EQ(PROP_NONE, propagate_not(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &A, NULL);
  EXPECT_CALL(*MockProxy, bind(&e, VALUE(1), NULL)).Times(1);
  EXPECT_EQ(1, propagate_not(&X, VALUE(0), NULL));
  delete(MockProxy);
}

TEST(PropagateAnd, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(0));
  struct constr_t B = CONSTRAINT_TERM(VALUE(1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_and(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &B, &A);
  EXPECT_EQ(PROP_NONE, propagate_and(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_and(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_and(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateAnd, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(VALUE(0));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_and(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_and(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_and(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(PropagateOr, Value) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(0));
  struct constr_t B = CONSTRAINT_TERM(VALUE(1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_or(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &B, &A);
  EXPECT_EQ(PROP_NONE, propagate_or(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_or(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_or(&X, VALUE(0), NULL));
  delete(MockProxy);
}

TEST(PropagateOr, Interval) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(VALUE(1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_or(&X, VALUE(1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_NONE, propagate_or(&X, INTERVAL(0, 1), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate_or(&X, VALUE(0), NULL));
  delete(MockProxy);
}

TEST(PropagateWand, Basic) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(0));
  struct constr_t B = CONSTRAINT_TERM(VALUE(1));
  struct wand_expr_t E [2] = { { .constr = &A, .orig = &A, .prop_tag = 0 }, { .constr = &B, .orig = &B, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_NONE, propagate_wand(&X, VALUE(0), NULL));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(PROP_ERROR, propagate_wand(&X, VALUE(1), NULL));
  delete(MockProxy);
}

TEST(Propagate, Loop) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(0));
  struct constr_t B = CONSTRAINT_TERM(VALUE(1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(PROP_ERROR, propagate(&X));
  delete(MockProxy);
}

} // end namespace
