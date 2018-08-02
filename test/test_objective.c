#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace objective {
#include "../src/arith.c"
#include "../src/objective.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(eval, const struct val_t(const struct constr_t *));
  MOCK_METHOD1(normalize, struct constr_t *(struct constr_t *constr));
  MOCK_METHOD2(propagate, struct constr_t *(struct constr_t *constr, struct val_t val));
  MOCK_METHOD2(print_fatal, void (const char *, va_list));
};

Mock *MockProxy;

const struct val_t eval(const struct constr_t *constr) {
  return MockProxy->eval(constr);
}

struct constr_t *normalize(struct constr_t *constr) {
  return MockProxy->normalize(constr);
}

struct constr_t *propagate(struct constr_t *constr, struct val_t val) {
  return MockProxy->propagate(constr, val);
}

void print_fatal(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_fatal(fmt, args);
  va_end(args);
}

TEST(ObjectiveInit, Basic) {
  domain_t best;

  objective_init(OBJ_ANY, &best);
  EXPECT_EQ(OBJ_ANY, _objective);
  EXPECT_EQ(&best, _objective_best);
  EXPECT_EQ(0, *_objective_best);

  objective_init(OBJ_ALL, &best);
  EXPECT_EQ(OBJ_ALL, _objective);
  EXPECT_EQ(&best, _objective_best);
  EXPECT_EQ(0, *_objective_best);

  objective_init(OBJ_MIN, &best);
  EXPECT_EQ(OBJ_MIN, _objective);
  EXPECT_EQ(&best, _objective_best);
  EXPECT_EQ(DOMAIN_MAX, *_objective_best);

  objective_init(OBJ_MAX, &best);
  EXPECT_EQ(OBJ_MAX, _objective);
  EXPECT_EQ(&best, _objective_best);
  EXPECT_EQ(DOMAIN_MIN, *_objective_best);
}

TEST(ObjectiveInit, Errors) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, testing::_)).Times(1);
  objective_init((objective_t)-1, NULL);
  delete(MockProxy);
}

TEST(Objective, Basic) {
  _objective = OBJ_MAX;
  EXPECT_EQ(OBJ_MAX, objective());
  _objective = OBJ_MIN;
  EXPECT_EQ(OBJ_MIN, objective());
}

TEST(ObjectiveBetter, Basic) {
  struct val_t a = VALUE(-1);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };

  domain_t best;
  _objective_best = &best;

  _objective = OBJ_ANY;
  *_objective_best = 17;
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));

  _objective = OBJ_ALL;
  *_objective_best = 17;
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));
}

TEST(ObjectiveBetter, Min) {
  struct val_t a = VALUE(-1);
  struct val_t b = VALUE(23);
  struct val_t c = INTERVAL(-1, 0);
  struct val_t d = INTERVAL(18, 23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t D = { .type = CONSTR_TERM, .constr = { .term = &d } };

  domain_t best;
  _objective_best = &best;

  _objective = OBJ_MIN;
  *_objective_best = DOMAIN_MAX;
  MockProxy = new Mock();
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));
  EXPECT_EQ(true, objective_better(&C));
  EXPECT_EQ(true, objective_better(&D));
  delete(MockProxy);

  _objective = OBJ_MIN;
  *_objective_best = 17;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&A))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(true, objective_better(&A));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&B))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(b));
  EXPECT_EQ(false, objective_better(&B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(c));
  EXPECT_EQ(true, objective_better(&C));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&D))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(d));
  EXPECT_EQ(false, objective_better(&D));
  delete(MockProxy);
}

TEST(ObjectiveBetter, Max) {
  struct val_t a = VALUE(-1);
  struct val_t b = VALUE(23);
  struct val_t c = INTERVAL(-1, 0);
  struct val_t d = INTERVAL(0, 23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct constr_t D = { .type = CONSTR_TERM, .constr = { .term = &d } };

  domain_t best;
  _objective_best = &best;

  _objective = OBJ_MAX;
  *_objective_best = DOMAIN_MIN;
  MockProxy = new Mock();
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));
  EXPECT_EQ(true, objective_better(&C));
  EXPECT_EQ(true, objective_better(&D));
  delete(MockProxy);

  _objective = OBJ_MAX;
  *_objective_best = 17;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&A))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(a));
  EXPECT_EQ(false, objective_better(&A));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&B))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(b));
  EXPECT_EQ(true, objective_better(&B));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(c));
  EXPECT_EQ(false, objective_better(&C));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&D))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(d));
  EXPECT_EQ(true, objective_better(&D));
  delete(MockProxy);
}

TEST(ObjectiveBetter, Errors) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, testing::_)).Times(1);
  _objective = (objective_t)-1;
  objective_better(NULL);
  delete(MockProxy);
}

TEST(ObjectiveUpdate, Basic) {
  domain_t best;
  _objective_best = &best;

  *_objective_best = -1;
  objective_update(VALUE(17));
  EXPECT_EQ(17, *_objective_best);
}

TEST(ObjectiveUpdate, Errors) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_UPDATE_BEST_WITH_INTERVAL, testing::_)).Times(1);
  objective_update(INTERVAL(0, 1));
  delete(MockProxy);
}

TEST(ObjectiveBest, Basic) {
  domain_t best;
  _objective_best = &best;

  *_objective_best = 23;
  EXPECT_EQ(23, objective_best());
  *_objective_best = 17;
  EXPECT_EQ(17, objective_best());
}

TEST(ObjectiveOptimize, Basic) {
  domain_t best;

  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);

  MockProxy = new Mock();
  objective_init(OBJ_ANY, &best);
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  objective_init(OBJ_ALL, &best);
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, Min) {
  domain_t best;

  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MIN, &best);
  objective_update(VALUE(17));
  EXPECT_CALL(*MockProxy, normalize(&X))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, propagate(&X, INTERVAL(DOMAIN_MIN, 16)))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(&X));
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, Max) {
  domain_t best;

  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MAX, &best);
  objective_update(VALUE(17));
  EXPECT_CALL(*MockProxy, normalize(&X))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(&X));
  EXPECT_CALL(*MockProxy, propagate(&X, INTERVAL(18, DOMAIN_MAX)))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(&X));
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, MinInfeas) {
  domain_t best;

  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MIN, &best);
  objective_update(VALUE(17));
  EXPECT_CALL(*MockProxy, propagate(&X, INTERVAL(DOMAIN_MIN, 16)))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, MaxInfeas) {
  domain_t best;

  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MAX, &best);
  objective_update(VALUE(17));
  EXPECT_CALL(*MockProxy, propagate(&X, INTERVAL(18, DOMAIN_MAX)))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(NULL, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, Error) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE, testing::_)).Times(1);
  _objective = (enum objective_t)-1;
  objective_update(VALUE(17));
  objective_optimize(&X);
  delete(MockProxy);
}

} // end namespace
