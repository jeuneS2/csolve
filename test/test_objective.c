#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace objective {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/objective.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(print_fatal, void (const char *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, const struct val_t(const struct constr_t *)); \
  MOCK_METHOD2(propagate_ ## NAME, prop_result_t(struct constr_t *, const struct val_t)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
const struct val_t eval_ ## NAME(const struct constr_t *constr) {       \
  return MockProxy->eval_ ## NAME(constr);                              \
}                                                                       \
prop_result_t propagate_ ## NAME(struct constr_t *constr, struct val_t val) { \
  return MockProxy->propagate_ ## NAME(constr, val);                    \
}                                                                       \
struct constr_t *normal_ ## NAME(struct constr_t *constr) {             \
  return MockProxy->normal_ ## NAME(constr);                            \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

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
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE)).Times(1);
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
  domain_t best;
  _objective_best = &best;

  _objective = OBJ_ANY;
  *_objective_best = 17;
  EXPECT_EQ(true, objective_better());
  EXPECT_EQ(true, objective_better());

  _objective = OBJ_ALL;
  *_objective_best = 17;
  EXPECT_EQ(true, objective_better());
  EXPECT_EQ(true, objective_better());
}

TEST(ObjectiveBetter, Min) {
  domain_t best;
  _objective_best = &best;

  _objective = OBJ_MIN;
  *_objective_best = DOMAIN_MAX;

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(-1));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(-1, 0));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(18, 23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  _objective = OBJ_MIN;
  *_objective_best = 17;

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(-1));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(23));
  EXPECT_EQ(false, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(-1, 0));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(18, 23));
  EXPECT_EQ(false, objective_better());
  delete(MockProxy);
}

TEST(ObjectiveBetter, Max) {
  domain_t best;
  _objective_best = &best;

  _objective = OBJ_MAX;
  *_objective_best = DOMAIN_MIN;
  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(-1));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(-1, 0));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(18, 23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  _objective = OBJ_MAX;
  *_objective_best = 17;
  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(-1));
  EXPECT_EQ(false, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(VALUE(23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(-1, 0));
  EXPECT_EQ(false, objective_better());
  delete(MockProxy);

  MockProxy = new Mock();
  _objective_val = CONSTRAINT_TERM(INTERVAL(18, 23));
  EXPECT_EQ(true, objective_better());
  delete(MockProxy);
}

TEST(ObjectiveBetter, Errors) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE)).Times(1);
  _objective = (objective_t)-1;
  objective_better();
  delete(MockProxy);
}

TEST(ObjectiveUpdateBest, Basic) {
  domain_t best;
  _objective_best = &best;

  _objective = OBJ_MAX;
  *_objective_best = -1;
  _objective_val = CONSTRAINT_TERM(VALUE(17));
  MockProxy = new Mock();
  objective_update_best();
  EXPECT_EQ(17, *_objective_best);
  delete(MockProxy);
}

TEST(ObjectiveUpdateBest, Errors) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OBJ_FUNC_TYPE)).Times(1);
  _objective = (objective_t)-1;
  objective_update_best();
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

} // end namespace
