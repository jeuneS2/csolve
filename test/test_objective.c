#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace objective {
#include "../src/arith.c"
#include "../src/objective.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(eval, const struct val_t(const struct constr_t *));
  MOCK_METHOD1(normalize, struct constr_t *(struct constr_t *constr));
  MOCK_METHOD2(propagate, struct constr_t *(struct constr_t *constr, struct val_t val));
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

TEST(ObjectiveInit, Basic) {
  objective_init(OBJ_ANY);
  EXPECT_EQ(OBJ_ANY, _objective);
  EXPECT_EQ(0, _objective_best);

  objective_init(OBJ_ALL);
  EXPECT_EQ(OBJ_ALL, _objective);
  EXPECT_EQ(0, _objective_best);

  objective_init(OBJ_MIN);
  EXPECT_EQ(OBJ_MIN, _objective);
  EXPECT_EQ(DOMAIN_MAX, _objective_best);

  objective_init(OBJ_MAX);
  EXPECT_EQ(OBJ_MAX, _objective);
  EXPECT_EQ(DOMAIN_MIN, _objective_best);
}

TEST(ObjectiveInit, Errors) {
  std::string output;

  testing::internal::CaptureStderr();
  objective_init((objective_t)-1);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: invalid objective function type: ffffffff\n");
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

  _objective = OBJ_ANY;
  _objective_best = 17;
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));

  _objective = OBJ_ALL;
  _objective_best = 17;
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

  _objective = OBJ_MIN;
  _objective_best = DOMAIN_MAX;
  MockProxy = new Mock();
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));
  EXPECT_EQ(true, objective_better(&C));
  EXPECT_EQ(true, objective_better(&D));
  delete(MockProxy);

  _objective = OBJ_MIN;
  _objective_best = 17;
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

  _objective = OBJ_MAX;
  _objective_best = DOMAIN_MIN;
  MockProxy = new Mock();
  EXPECT_EQ(true, objective_better(&A));
  EXPECT_EQ(true, objective_better(&B));
  EXPECT_EQ(true, objective_better(&C));
  EXPECT_EQ(true, objective_better(&D));
  delete(MockProxy);

  _objective = OBJ_MAX;
  _objective_best = 17;
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
  std::string output;

  testing::internal::CaptureStderr();
  _objective = (objective_t)-1;
  objective_better(NULL);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: invalid objective function type: ffffffff\n");
}

TEST(ObjectiveUpdate, Basic) {
  _objective_best = -1;
  objective_update(VALUE(17));
  EXPECT_EQ(17, _objective_best);
}

TEST(ObjectiveUpdate, Errors) {
  std::string output;

  testing::internal::CaptureStderr();
  objective_update(INTERVAL(0, 1));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: trying to update best value with interval\n");
}

TEST(ObjectiveBest, Basic) {
  _objective_best = 23;
  EXPECT_EQ(23, objective_best());
  _objective_best = 17;
  EXPECT_EQ(17, objective_best());
}

TEST(ObjectiveOptimize, Basic) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);

  MockProxy = new Mock();
  objective_init(OBJ_ANY);
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  objective_init(OBJ_ALL);
  EXPECT_EQ(&X, objective_optimize(&X));
  delete(MockProxy);
}

TEST(ObjectiveOptimize, Min) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MIN);
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
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  objective_init(OBJ_MAX);
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

TEST(ObjectiveOptimize, Error) {
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;

  std::string output;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  MockProxy = new Mock();
  _objective = (enum objective_t)-1;
  objective_update(VALUE(17));
  testing::internal::CaptureStderr();
  EXPECT_EQ(&X, objective_optimize(&X));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: invalid objective function type: ffffffff\n");
  delete(MockProxy);
}

} // end namespace
