#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace csolve {
#include "../src/stats.c"
#include "../src/constr_types.c"
#include "../src/csolve.c"

#define STR(X) #X
#define STRVAL(X) STR(X)

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

bool operator==(const struct wand_expr_t& lhs, const struct wand_expr_t& rhs) {
  return lhs.constr == rhs.constr && lhs.prop_tag == rhs.prop_tag;
}

class Mock {
 public:
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD1(dealloc, void(void *));
  MOCK_METHOD0(cache_clean, void(void));
  MOCK_METHOD2(bind, size_t(struct val_t *, const struct val_t));
  MOCK_METHOD1(unbind, void(size_t));
  MOCK_METHOD2(patch, size_t(struct wand_expr_t *, const struct wand_expr_t));
  MOCK_METHOD1(unpatch, void(size_t));
  MOCK_METHOD1(sema_init, void(sem_t *));
  MOCK_METHOD1(sema_wait, void(sem_t *));
  MOCK_METHOD1(sema_post, void(sem_t *));
  MOCK_METHOD1(normal, struct constr_t *(struct constr_t *));
  MOCK_METHOD1(propagate_clauses, prop_result_t(const struct clause_list_t *));
  MOCK_METHOD0(objective, enum objective_t(void));
  MOCK_METHOD0(objective_better, bool(void));
  MOCK_METHOD0(objective_update_best, void(void));
  MOCK_METHOD0(objective_update_val, void(void));
  MOCK_METHOD0(objective_val, struct constr_t*(void));
  MOCK_METHOD0(strategy_restart_frequency, uint64_t(void));
  MOCK_METHOD0(strategy_var_order_pop, struct env_t *(void));
  MOCK_METHOD1(strategy_var_order_push, void(struct env_t *));
  MOCK_METHOD1(print_error, void(const char *));
  MOCK_METHOD3(print_solution, void(FILE *, size_t, struct env_t *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, const struct val_t(const struct constr_t *)); \
  MOCK_METHOD2(propagate_ ## NAME, prop_result_t(struct constr_t *, const struct val_t)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

void *alloc(size_t size) {
  return MockProxy->alloc(size);
}

void dealloc(void *elem) {
  MockProxy->dealloc(elem);
}

void cache_clean(void) {
  MockProxy->cache_clean();
}

size_t bind(struct val_t *loc, const struct val_t val) {
  return MockProxy->bind(loc, val);
}

void unbind(size_t depth) {
  MockProxy->unbind(depth);
}

size_t patch(struct wand_expr_t *loc, const struct wand_expr_t val) {
  return MockProxy->patch(loc, val);
}

void unpatch(size_t depth) {
  MockProxy->unpatch(depth);
}

void sema_init(sem_t *sema) {
  MockProxy->sema_init(sema);
}

void sema_wait(sem_t *sema) {
  MockProxy->sema_wait(sema);
}

void sema_post(sem_t *sema) {
  MockProxy->sema_post(sema);
}

struct constr_t *normal(struct constr_t *constr) {
  return MockProxy->normal(constr);
}

prop_result_t propagate_clauses(const struct clause_list_t *clauses) {
  return MockProxy->propagate_clauses(clauses);
}

enum objective_t objective(void) {
  return MockProxy->objective();
}

uint64_t strategy_restart_frequency(void) {
  return MockProxy->strategy_restart_frequency();
}

struct env_t *strategy_var_order_pop(void) {
  return MockProxy->strategy_var_order_pop();
}

void strategy_var_order_push(struct env_t *var) {
  MockProxy->strategy_var_order_push(var);
}

bool objective_better() {
  return MockProxy->objective_better();
}

void objective_update_best() {
  MockProxy->objective_update_best();
}

void objective_update_val() {
  MockProxy->objective_update_val();
}

struct constr_t *objective_val(void) {
  return MockProxy->objective_val();
}

void print_error(const char *fmt, ...) {
  MockProxy->print_error(fmt);
}

void print_solution(FILE *file, size_t size, struct env_t *env) {
  MockProxy->print_solution(file, size, env);
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

TEST(Stats, Init) {
  stats_init();
  EXPECT_EQ(0, calls);
  EXPECT_EQ(0, cuts);
  EXPECT_EQ(0, cut_depth);
  EXPECT_EQ(0, restarts);
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
  EXPECT_EQ(0, alloc_max);
}

TEST(Stats, Update) {
  stats_init();
  update_stats(17);
  EXPECT_EQ(depth_min, 17);
  EXPECT_EQ(depth_max, 17);
  EXPECT_EQ(calls, 1);
  update_stats(16);
  EXPECT_EQ(depth_min, 16);
  EXPECT_EQ(depth_max, 17);
  EXPECT_EQ(calls, 2);
  update_stats(18);
  EXPECT_EQ(depth_min, 16);
  EXPECT_EQ(depth_max, 18);
  EXPECT_EQ(calls, 3);
}

TEST(Stats, Print) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = STATS_FREQUENCY-1;
  cuts = 3;
  props = 4;
  restarts = 6;
  depth_min = 7;
  depth_max = 8;
  cut_depth = 9;
  alloc_max = 10;
  _shared->solutions = 11;

  std::string output;
  testing::internal::CaptureStdout();
  update_stats(7);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "#1: CALLS: " STRVAL(STATS_FREQUENCY) ", CUTS: 3, PROPS: 4, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 3.000000, MEMORY: 10, SOLUTIONS: 11\n");
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
}

TEST(PrintStats, Stdout) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = 2;
  cuts = 3;
  props = 4;
  restarts = 6;
  depth_min = 7;
  depth_max = 8;
  cut_depth = 9;
  alloc_max = 10;
  _shared->solutions = 11;

  std::string output;
  testing::internal::CaptureStdout();
  print_stats(stdout);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "#1: CALLS: 2, CUTS: 3, PROPS: 4, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 3.000000, MEMORY: 10, SOLUTIONS: 11\n");
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
}

TEST(PrintStats, Stderr) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = 2;
  cuts = 3;
  props = 4;
  restarts = 6;
  depth_min = 7;
  depth_max = 8;
  cut_depth = 9;
  alloc_max = 10;
  _shared->solutions = 11;

  std::string output;
  testing::internal::CaptureStderr();
  print_stats(stderr);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "#1: CALLS: 2, CUTS: 3, PROPS: 4, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 3.000000, MEMORY: 10, SOLUTIONS: 11\n");
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
}

TEST(Shared, Init) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sema_init(testing::_))
    .Times(1);
  shared_init(7);
  EXPECT_EQ(7, _workers_max);
  EXPECT_NE((struct shared_t *)NULL, _shared);
  EXPECT_EQ(1, shared()->workers);
  EXPECT_EQ(1, shared()->workers_id);
  EXPECT_EQ(1, _worker_id);
  EXPECT_EQ(0, _worker_min_depth);
  delete(MockProxy);
}

TEST(Shared, Get) {
  struct shared_t s;
  _shared = &s;

  EXPECT_EQ(&s, shared());
}

TEST(FailThresholdNext, Basic) {
  _fail_threshold = 1;
  _fail_threshold_counter = 1;
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(2, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(2, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(4, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(2, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(1, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(2, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(4, _fail_threshold);
  fail_threshold_next();
  EXPECT_EQ(8, _fail_threshold);
}

TEST(UpdateSolution, FalseConstr) {
  struct constr_t C = CONSTRAINT_TERM(VALUE(0));
  struct env_t env[0];

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_term(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(0)));
  EXPECT_EQ(false, update_solution(0, env, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, AnySolution) {
  struct constr_t C = CONSTRAINT_TERM(VALUE(0));
  struct env_t env[0];

  struct shared_t s;
  s.solutions = 1;
  _shared = &s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_term(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_EQ(false, update_solution(0, env, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, NotBetter) {
  struct constr_t C = CONSTRAINT_TERM(VALUE(0));
  struct env_t env[0];

  struct shared_t s;
  s.solutions = 0;
  _shared = &s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_term(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ALL));
  EXPECT_CALL(*MockProxy, objective_better())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_EQ(false, update_solution(0, env, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, Better) {
  struct constr_t C = CONSTRAINT_TERM(VALUE(0));
  struct env_t env[0];

  struct shared_t s;
  s.solutions = 0;
  _shared = &s;

  _worker_id = 17;

  std::string output;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_term(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_CALL(*MockProxy, objective_better())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*MockProxy, objective_update_best())
    .Times(::testing::AtLeast(1));
  EXPECT_CALL(*MockProxy, print_solution(stdout, 0, env))
    .Times(1);
  testing::internal::CaptureStderr();
  EXPECT_EQ(true, update_solution(0, env, &C));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ("#17: ", output);
  EXPECT_EQ(1, s.solutions);
  delete(MockProxy);
}

TEST(CheckAssignment, Infeasible) {
  struct constr_t c = CONSTRAINT_TERM(VALUE(1));
  struct env_t e = { .key = NULL, .val = &c, .clauses = { .length = 0, .elems = NULL }, .order = 0, .prio = 0 };

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, propagate_clauses(&e.clauses))
    .Times(1)
    .WillRepeatedly(::testing::Return(PROP_ERROR));
  EXPECT_EQ(true, check_assignment(&e, 0));
  EXPECT_EQ(1, cuts);
  delete(MockProxy);
}

TEST(CheckAssignment, Feasible) {
  struct constr_t c = CONSTRAINT_TERM(VALUE(1));
  struct env_t e = { .key = NULL, .val = &c, .clauses = { .length = 0, .elems = NULL }, .order = 0, .prio = 0 };
  struct constr_t obj = CONSTRAINT_TERM(VALUE(0));

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, propagate_clauses(&e.clauses))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(PROP_NONE));
  EXPECT_CALL(*MockProxy, objective_val())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(&obj));
  EXPECT_EQ(false, check_assignment(&e, 0));
  delete(MockProxy);
}

TEST(CheckRestart, WrongStrategy) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_EQ(false, check_restart());
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(OBJ_ALL));
  EXPECT_EQ(false, check_restart());
  delete(MockProxy);
}

TEST(CheckRestart, False) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  _fail_count = 1;
  _fail_threshold = 10;
  EXPECT_EQ(false, check_restart());
  EXPECT_EQ(2, _fail_count);
  delete(MockProxy);
}

TEST(CheckRestart, True) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  _fail_count = 10;
  _fail_threshold = 1;
  EXPECT_EQ(true, check_restart());
  EXPECT_EQ(0, _fail_count);
  delete(MockProxy);
}

TEST(Step, Activate) {
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(12, 13));
  struct env_t e = { .key = NULL, .val = &c, .clauses = { .length = 0, .elems = NULL }, .order = 0, .prio = 0 };
  struct step_t s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(0))
    .WillOnce(::testing::Return(0));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  s.active = false;
  step_activate(&s, &e);
  EXPECT_EQ(true, s.active);
  EXPECT_EQ(&e, s.var);
  EXPECT_EQ(INTERVAL(12, 13), s.bounds);
  EXPECT_EQ(0, s.iter);
  EXPECT_EQ(0, s.seed);
  delete(MockProxy);
}

TEST(Step, Deactivate) {
  struct constr_t c;
  struct env_t e = { .key = NULL, .val = &c, .clauses = { .length = 0, .elems = NULL }, .order = 0, .prio = 0 };
  struct step_t s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_var_order_push(&e))
    .Times(1);
  s.var = &e;
  s.active = true;
  step_deactivate(&s);
  EXPECT_EQ(false, s.active);
  delete(MockProxy);
}

TEST(Step, Enter) {
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(0, 100));
  struct env_t e = { .key = NULL, .val = &c, .clauses = { .length = 0, .elems = NULL }, .order = 0, .prio = 0 };
  struct step_t s;
  s.var = &e;

  char marker;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(0))
    .Times(1)
    .WillOnce(::testing::Return(&marker));
  EXPECT_CALL(*MockProxy, patch(NULL, (struct wand_expr_t){ NULL, 0 }))
    .Times(1)
    .WillOnce(::testing::Return(17));
  EXPECT_CALL(*MockProxy, bind(&c.constr.term.val, VALUE(42)))
    .Times(1)
    .WillOnce(::testing::Return(23));
  step_enter(&s, 42);
  EXPECT_EQ(&marker, s.alloc_marker);
  EXPECT_EQ(23, s.bind_depth);
  EXPECT_EQ(17, s.patch_depth);
  delete(MockProxy);
}

TEST(Step, Leave) {
  struct step_t s;

  char marker;

  s.bind_depth = 17;
  s.patch_depth = 23;
  s.alloc_marker = &marker;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, unbind(17))
    .Times(1);
  EXPECT_CALL(*MockProxy, unpatch(23))
    .Times(1);
  EXPECT_CALL(*MockProxy, dealloc(&marker))
    .Times(1);
  step_leave(&s);
  delete(MockProxy);
}

TEST(Step, Next) {
  struct step_t s;

  s.iter = 23;

  MockProxy = new Mock();
  step_next(&s);
  EXPECT_EQ(s.iter, 24);
  delete(MockProxy);
}

TEST(Step, Check) {
  struct val_t v = INTERVAL(3, 17);
  struct step_t s;

  s.bounds = v;
  s.iter = 4;
  EXPECT_EQ(true, step_check(&s));
  s.iter = 14;
  EXPECT_EQ(true, step_check(&s));
  s.iter = 15;
  EXPECT_EQ(false, step_check(&s));
}

TEST(Step, Val) {
  struct val_t v = INTERVAL(3, 17);
  struct step_t s;

  s.bounds = v;

  s.iter = 4;
  domain_t v1 = step_val(&s);
  EXPECT_LE(3, v1);
  EXPECT_GE(17, v1);

  s.iter = 5;
  domain_t v2 = step_val(&s);
  EXPECT_LE(3, v2);
  EXPECT_GE(17, v2);
  EXPECT_NE(v1, v2);
}

}
