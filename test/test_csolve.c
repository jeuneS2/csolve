#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace csolve {
#include "../src/csolve.c"

#define STR(X) #X
#define STRVAL(X) STR(X)

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(alloc, void *(size_t));
  MOCK_METHOD1(dealloc, void(void *));
  MOCK_METHOD2(bind, size_t(struct val_t *, const struct val_t));
  MOCK_METHOD1(unbind, void(size_t));
  MOCK_METHOD1(sema_init, void(sem_t *));
  MOCK_METHOD1(sema_wait, void(sem_t *));
  MOCK_METHOD1(sema_post, void(sem_t *));
  MOCK_METHOD1(eval, const struct val_t(const struct constr_t *));
  MOCK_METHOD1(normalize, struct constr_t *(struct constr_t *));
  MOCK_METHOD2(propagate, struct constr_t *(struct constr_t *, struct val_t));
  MOCK_METHOD0(objective, enum objective_t(void));
  MOCK_METHOD1(objective_better, bool(struct constr_t *));
  MOCK_METHOD1(objective_update, void(struct val_t));
  MOCK_METHOD1(objective_optimize, struct constr_t *(struct constr_t *));
  MOCK_METHOD0(strategy_restart_frequency, uint64_t(void));
  MOCK_METHOD2(strategy_pick_var, void(struct env_t *, size_t));
  MOCK_METHOD2(print_error, void(const char *, va_list));
  MOCK_METHOD2(print_solution, void(FILE *, struct env_t *));
};

Mock *MockProxy;

size_t alloc_max;

void *alloc(size_t size) {
  return MockProxy->alloc(size);
}

void dealloc(void *elem) {
  MockProxy->dealloc(elem);
}

size_t bind(struct val_t *loc, const struct val_t val) {
  return MockProxy->bind(loc, val);
}

void unbind(size_t depth) {
  MockProxy->unbind(depth);
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

const struct val_t eval(const struct constr_t *constr) {
  return MockProxy->eval(constr);
}

struct constr_t *normalize(struct constr_t *constr) {
  return MockProxy->normalize(constr);
}

struct constr_t *propagate(struct constr_t *constr, struct val_t val) {
  return MockProxy->propagate(constr, val);
}

enum objective_t objective(void) {
  return MockProxy->objective();
}

uint64_t strategy_restart_frequency(void) {
  return MockProxy->strategy_restart_frequency();
}

void strategy_pick_var(struct env_t *env, size_t depth) {
  return MockProxy->strategy_pick_var(env, depth);
}

bool objective_better(struct constr_t *obj) {
  return MockProxy->objective_better(obj);
}

void objective_update(struct val_t obj) {
  MockProxy->objective_update(obj);
}

struct constr_t *objective_optimize(struct constr_t *obj) {
  return MockProxy->objective_optimize(obj);
}

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

void print_solution(FILE *file, struct env_t *env) {
  MockProxy->print_solution(file, env);
}

TEST(Stats, Init) {
  stats_init();
  EXPECT_EQ(0, calls);
  EXPECT_EQ(0, cuts_prop);
  EXPECT_EQ(0, cuts_bound);
  EXPECT_EQ(0, cuts_obj);
  EXPECT_EQ(0, cut_depth);
  EXPECT_EQ(0, restarts);
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
  EXPECT_EQ(0, alloc_max);
}

TEST(Stats, Update) {
  stats_init();
  stats_update(17);
  EXPECT_EQ(depth_min, 17);
  EXPECT_EQ(depth_max, 17);
  EXPECT_EQ(calls, 1);
  stats_update(16);
  EXPECT_EQ(depth_min, 16);
  EXPECT_EQ(depth_max, 17);
  EXPECT_EQ(calls, 2);
  stats_update(18);
  EXPECT_EQ(depth_min, 16);
  EXPECT_EQ(depth_max, 18);
  EXPECT_EQ(calls, 3);
}

TEST(Stats, Print) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = STATS_FREQUENCY-1;
  cuts_prop = 3;
  cuts_obj = 4;
  cuts_bound = 5;
  restarts = 6;
  depth_min = 7;
  depth_max = 8;
  cut_depth = 9;
  alloc_max = 10;
  _shared->solutions = 11;

  std::string output;
  testing::internal::CaptureStdout();
  stats_update(7);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "#1: CALLS: " STRVAL(STATS_FREQUENCY) ", CUTS: 3/4, BOUND: 5, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 0.750000, MEMORY: 10, SOLUTIONS: 11\n");
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
}

TEST(PrintStats, Stdout) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = 2;
  cuts_prop = 3;
  cuts_obj = 4;
  cuts_bound = 5;
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
  EXPECT_EQ(output, "#1: CALLS: 2, CUTS: 3/4, BOUND: 5, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 0.750000, MEMORY: 10, SOLUTIONS: 11\n");
  EXPECT_EQ(SIZE_MAX, depth_min);
  EXPECT_EQ(0, depth_max);
}

TEST(PrintStats, Stderr) {
  struct shared_t s;
  _shared = &s;

  _worker_id = 1;
  calls = 2;
  cuts_prop = 3;
  cuts_obj = 4;
  cuts_bound = 5;
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
  EXPECT_EQ(output, "#1: CALLS: 2, CUTS: 3/4, BOUND: 5, RESTARTS: 6, DEPTH: 7/8, AVG DEPTH: 0.750000, MEMORY: 10, SOLUTIONS: 11\n");
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
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(2, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(2, fail_threshold_next());
  EXPECT_EQ(4, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(2, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(1, fail_threshold_next());
  EXPECT_EQ(2, fail_threshold_next());
  EXPECT_EQ(4, fail_threshold_next());
  EXPECT_EQ(8, fail_threshold_next());
}

TEST(SwapEnv, Basic) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  swap_env(env, 0, 1);
  EXPECT_STREQ("b", env[0].key);
  EXPECT_EQ(&b, env[0].val);
  EXPECT_EQ(&sA, env[0].step);
  EXPECT_STREQ("a", env[1].key);
  EXPECT_EQ(&a, env[1].val);
  EXPECT_EQ(&sB, env[1].step);
  delete(MockProxy);
}

TEST(SwapEnv, NoSwap) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  swap_env(env, 0, 2);
  EXPECT_STREQ("a", env[0].key);
  EXPECT_EQ(&a, env[0].val);
  EXPECT_EQ(&sA, env[0].step);
  EXPECT_STREQ("b", env[1].key);
  EXPECT_EQ(&b, env[1].val);
  EXPECT_EQ(&sB, env[1].step);
  EXPECT_EQ((const char *)NULL, env[2].key);
  EXPECT_EQ((const val_t *)NULL, env[2].val);
  EXPECT_EQ((const solve_step_t *)NULL, env[2].step);
  delete(MockProxy);

  MockProxy = new Mock();
  swap_env(env, 2, 0);
  EXPECT_STREQ("a", env[0].key);
  EXPECT_EQ(&a, env[0].val);
  EXPECT_EQ(&sA, env[0].step);
  EXPECT_STREQ("b", env[1].key);
  EXPECT_EQ(&b, env[1].val);
  EXPECT_EQ(&sB, env[1].step);
  EXPECT_EQ((const char *)NULL, env[2].key);
  EXPECT_EQ((const val_t *)NULL, env[2].val);
  EXPECT_EQ((const solve_step_t *)NULL, env[2].step);
  delete(MockProxy);

  MockProxy = new Mock();
  swap_env(env, 1, 1);
  EXPECT_EQ("a", env[0].key);
  EXPECT_EQ(&a, env[0].val);
  EXPECT_EQ(&sA, env[0].step);
  EXPECT_EQ("b", env[1].key);
  EXPECT_EQ(&b, env[1].val);
  EXPECT_EQ(&sB, env[1].step);
  EXPECT_EQ((const char *)NULL, env[2].key);
  EXPECT_EQ((const val_t *)NULL, env[2].val);
  EXPECT_EQ((const solve_step_t *)NULL, env[2].step);
  delete(MockProxy);
}

TEST(UpdateSolution, FalseConstr) {
  struct val_t c = VALUE(0);
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct val_t o = VALUE(0);
  struct constr_t O = { .type = CONSTR_TERM, .constr = { .term = &o } };
  struct env_t env[1];
  env[0] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(0)));
  EXPECT_EQ(false, update_solution(env, &O, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, AnySolution) {
  struct val_t c = VALUE(0);
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct val_t o = VALUE(0);
  struct constr_t O = { .type = CONSTR_TERM, .constr = { .term = &o } };
  struct env_t env[1];
  env[0] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  struct shared_t s;
  s.solutions = 1;
  _shared = &s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_EQ(false, update_solution(env, &O, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, NotBetter) {
  struct val_t c = VALUE(0);
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct val_t o = VALUE(0);
  struct constr_t O = { .type = CONSTR_TERM, .constr = { .term = &o } };
  struct env_t env[1];
  env[0] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  struct shared_t s;
  s.solutions = 0;
  _shared = &s;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ALL));
  EXPECT_CALL(*MockProxy, objective_better(&O))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(false));
  EXPECT_EQ(false, update_solution(env, &O, &C));
  delete(MockProxy);
}

TEST(UpdateSolution, Better) {
  struct val_t c = VALUE(0);
  struct constr_t C = { .type = CONSTR_TERM, .constr = { .term = &c } };
  struct val_t o = VALUE(0);
  struct constr_t O = { .type = CONSTR_TERM, .constr = { .term = &o } };
  struct env_t env[1];
  env[0] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  struct shared_t s;
  s.solutions = 0;
  _shared = &s;

  _worker_id = 17;

  std::string output;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval(&C))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(1)));
  EXPECT_CALL(*MockProxy, sema_wait(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, sema_post(&s.semaphore))
    .Times(1);
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_CALL(*MockProxy, objective_better(&O))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(true));
  EXPECT_CALL(*MockProxy, eval(&O))
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(VALUE(23)));
  EXPECT_CALL(*MockProxy, objective_update(VALUE(23)))
    .Times(::testing::AtLeast(1));
  EXPECT_CALL(*MockProxy, print_solution(stderr, env))
    .Times(1);
  testing::internal::CaptureStderr();
  EXPECT_EQ(true, update_solution(env, &O, &C));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ("#17: ", output);
  EXPECT_EQ(1, s.solutions);
  delete(MockProxy);
}

TEST(CheckAssignment, NotBetter) {
  struct env_t env[3];

  struct val_t a = VALUE(1);
  struct solve_step_t sA;
  struct constr_t oA;
  sA.obj = &oA;
  env[0] = { .key = "a", .val = &a, .fails = 2, .step = &sA };

  struct val_t b = INTERVAL(3, 4);
  struct solve_step_t sB;
  struct constr_t oB;
  sB.obj = &oB;
  env[1] = { .key = "b", .val = &b, .fails = 5, .step = &sB };

  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_better(&oA))
    .Times(1)
    .WillOnce(::testing::Return(false));
  EXPECT_EQ(true, check_assignment(env, 0));
  EXPECT_EQ(1, cuts_bound);
  delete(MockProxy);
}

TEST(CheckAssignment, InfeasibleObj) {
  struct env_t env[3];

  struct val_t a = VALUE(1);
  struct solve_step_t sA;
  struct constr_t oA;
  sA.obj = &oA;
  env[0] = { .key = "a", .val = &a, .fails = 2, .step = &sA };

  struct val_t b = INTERVAL(3, 4);
  struct solve_step_t sB;
  struct constr_t oB;
  sB.obj = &oB;
  env[1] = { .key = "b", .val = &b, .fails = 5, .step = &sB };

  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_better(&oA))
    .Times(1)
    .WillOnce(::testing::Return(true));
  EXPECT_CALL(*MockProxy, objective_optimize(&oA))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(true, check_assignment(env, 0));
  EXPECT_EQ(1, cuts_obj);
  delete(MockProxy);
}

TEST(CheckAssignment, Infeasible) {
  struct env_t env[3];

  struct val_t a = VALUE(1);
  struct solve_step_t sA;
  struct constr_t oA;
  struct constr_t cA;
  sA.obj = &oA;
  sA.constr = &cA;
  env[0] = { .key = "a", .val = &a, .fails = 2, .step = &sA };

  struct val_t b = INTERVAL(3, 4);
  struct solve_step_t sB;
  struct constr_t oB;
  struct constr_t cB;
  sB.obj = &oB;
  sB.constr = &cB;
  env[1] = { .key = "b", .val = &b, .fails = 5, .step = &sB };

  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_better(&oA))
    .Times(1)
    .WillOnce(::testing::Return(true));
  EXPECT_CALL(*MockProxy, objective_optimize(&oA))
    .Times(1)
    .WillOnce(::testing::Return(&oA));
  EXPECT_CALL(*MockProxy, propagate(&cA, VALUE(1)))
    .Times(1)
    .WillOnce(::testing::Return((struct constr_t *)NULL));
  EXPECT_EQ(true, check_assignment(env, 0));
  EXPECT_EQ(1, cuts_prop);
  delete(MockProxy);
}

TEST(CheckAssignment, Feasible) {
  struct env_t env[3];

  struct val_t a = VALUE(1);
  struct solve_step_t sA;
  struct constr_t oA;
  struct constr_t cA;
  sA.obj = &oA;
  sA.constr = &cA;
  env[0] = { .key = "a", .val = &a, .fails = 2, .step = &sA };

  struct val_t b = INTERVAL(3, 4);
  struct solve_step_t sB;
  struct constr_t oB;
  struct constr_t cB;
  sB.obj = &oB;
  sB.constr = &cB;
  env[1] = { .key = "b", .val = &b, .fails = 5, .step = &sB };

  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  stats_init();

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_better(&oA))
    .Times(1)
    .WillOnce(::testing::Return(true));
  EXPECT_CALL(*MockProxy, objective_optimize(&oA))
    .Times(1)
    .WillOnce(::testing::Return(&oA));
  EXPECT_CALL(*MockProxy, propagate(&cA, VALUE(1)))
    .Times(1)
    .WillOnce(::testing::Return(&cA));
  EXPECT_CALL(*MockProxy, normalize(&cA))
    .Times(1)
    .WillOnce(::testing::Return(&cA));
  EXPECT_EQ(false, check_assignment(env, 0));
  EXPECT_EQ(env[1].step->obj, &oA);
  EXPECT_EQ(env[1].step->constr, &cA);
  delete(MockProxy);
}

TEST(CheckRestart, WrongStrategy) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(0));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(0))
    .WillRepeatedly(::testing::Return(OBJ_ANY));
  EXPECT_EQ(false, check_restart());
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(1));
  EXPECT_CALL(*MockProxy, objective())
    .Times(::testing::AtLeast(1))
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
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, strategy_restart_frequency())
    .Times(1)
    .WillOnce(::testing::Return(0));
  env[1].step->active = false;
  step_activate(env, 1);
  EXPECT_EQ(true, env[1].step->active);
  EXPECT_EQ(*env[1].val, env[1].step->bounds);
  EXPECT_EQ(0, env[1].step->iter);
  EXPECT_EQ(0, env[1].step->seed);
  delete(MockProxy);
}

TEST(Step, Deactivate) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  env[1].step->active = true;
  step_deactivate(env, 1);
  EXPECT_EQ(false, env[1].step->active);
  delete(MockProxy);
}

TEST(Step, Enter) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  char marker;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, alloc(0))
    .Times(1)
    .WillOnce(::testing::Return(&marker));
  EXPECT_CALL(*MockProxy, bind(env[1].val, VALUE(42)))
    .Times(1)
    .WillOnce(::testing::Return(23));
  step_enter(env, 1, 42);
  EXPECT_EQ(&marker, env[1].step->alloc_marker);
  EXPECT_EQ(23, env[1].step->bind_depth);
  delete(MockProxy);
}

TEST(Step, Leave) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  char marker;

  env[1].step->bind_depth = 17;
  env[1].step->alloc_marker = &marker;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, unbind(17))
    .Times(1);
  EXPECT_CALL(*MockProxy, dealloc(&marker))
    .Times(1);
  step_leave(env, 1);
  delete(MockProxy);
}

TEST(Step, Next) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  env[1].step->iter = 23;

  MockProxy = new Mock();
  step_next(env, 1);
  EXPECT_EQ(env[1].step->iter, 24);
  delete(MockProxy);
}

TEST(Step, Check) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  env[1].step->bounds = *env[1].val;
  env[1].step->iter = 4;
  EXPECT_EQ(true, step_check(env, 1));
  env[1].step->iter = 14;
  EXPECT_EQ(true, step_check(env, 1));
  env[1].step->iter = 15;
  EXPECT_EQ(false, step_check(env, 1));
}

TEST(Step, Val) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  env[1].step->bounds = *env[1].val;

  env[1].step->iter = 4;
  domain_t v1 = step_val(env, 1);
  EXPECT_LE(3, v1);
  EXPECT_GE(17, v1);

  env[1].step->iter = 5;
  domain_t v2 = step_val(env, 1);
  EXPECT_LE(3, v2);
  EXPECT_GE(17, v2);
  EXPECT_NE(v1, v2);
}

}
