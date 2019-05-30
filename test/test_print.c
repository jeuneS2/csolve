#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace print {

void mock_exit(int code);
#define exit mock_exit

#include "../src/constr_types.c"
#include "../src/print.c"

class Mock {
 public:
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD0(main_name, const char *(void));
  MOCK_METHOD1(exit, void(int));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, struct val_t(const struct constr_t *)); \
  MOCK_METHOD3(propagate_ ## NAME, prop_result_t(struct constr_t *, struct val_t, const struct wand_expr_t *)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

domain_t objective_best(void) {
  return MockProxy->objective_best();
}

const char *main_name(void) {
  return MockProxy->main_name();
}

void exit(int code) {
  return MockProxy->exit(code);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
struct val_t eval_ ## NAME(const struct constr_t *constr) {       \
  return MockProxy->eval_ ## NAME(constr);                              \
}                                                                       \
prop_result_t propagate_ ## NAME(struct constr_t *constr, struct val_t val, const struct wand_expr_t *clause) { \
  return MockProxy->propagate_ ## NAME(constr, val, clause);            \
}                                                                       \
struct constr_t *normal_ ## NAME(struct constr_t *constr) {             \
  return MockProxy->normal_ ## NAME(constr);                            \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)

TEST(PrintValue, Basic) {
  std::string output;

  testing::internal::CaptureStderr();
  print_val(stderr, VALUE(17));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " 17");

  testing::internal::CaptureStderr();
  print_val(stderr, INTERVAL(-3, 2));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " [-3;2]");

  testing::internal::CaptureStdout();
  print_val(stdout, VALUE(17));
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " 17");

  testing::internal::CaptureStdout();
  print_val(stdout, INTERVAL(-3, 2));
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " [-3;2]");
}

TEST(PrintConstr, Basic) {
  std::string output;

  struct constr_t A = CONSTRAINT_TERM(VALUE(17));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(23,42));
  struct constr_t X;

  testing::internal::CaptureStderr();
  print_constr(stderr, &A);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " 17");

  testing::internal::CaptureStdout();
  print_constr(stdout, &A);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " 17");

  testing::internal::CaptureStderr();
  print_constr(stderr, &B);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " [23;42]");

  testing::internal::CaptureStdout();
  print_constr(stdout, &B);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " [23;42]");

  X = CONSTRAINT_EXPR(EQ, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (= 17 [23;42])");

  X = CONSTRAINT_EXPR(EQ, &A, &B);
  testing::internal::CaptureStdout();
  print_constr(stdout, &X);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " (= 17 [23;42])");

  X = CONSTRAINT_EXPR(LT, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (< 17 [23;42])");

  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (- 17)");

  X = CONSTRAINT_EXPR(ADD, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (+ [23;42] 17)");

  X = CONSTRAINT_EXPR(MUL, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (* [23;42] 17)");

  X = CONSTRAINT_EXPR(AND, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (& 17 [23;42])");

  X = CONSTRAINT_EXPR(OR, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (| [23;42] 17)");
}

TEST(PrintConstr, Wand) {
  std::string output;

  struct constr_t A = CONSTRAINT_TERM(VALUE(17));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(23,42));
  struct wand_expr_t E [2] = { { .constr = &A, .orig = &A, .prop_tag = 0 }, { .constr = &B, .orig = &B, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " 17; [23;42];");
}

TEST(PrintEnv, Basic) {
  std::string output;
  struct constr_t a = CONSTRAINT_TERM(VALUE(17));
  struct constr_t b = CONSTRAINT_TERM(VALUE(23));
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(-1,42));

  struct env_t env [3] = { { "a", &a },
                           { "b", &b },
                           { "c", &c } };

  testing::internal::CaptureStderr();
  print_env(stderr, 3, env);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "a = 17, b = 23, c = [-1;42], ");

  testing::internal::CaptureStdout();
  print_env(stdout, 3, env);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "a = 17, b = 23, c = [-1;42], ");
}

TEST(PrintSolution, Basic) {
  std::string output;
  struct constr_t a = CONSTRAINT_TERM(VALUE(17));
  struct constr_t b = CONSTRAINT_TERM(VALUE(23));
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(-1,42));

  struct env_t env [3] = { { "a", &a },
                           { "b", &b },
                           { "c", &c } };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_best())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(77));
  testing::internal::CaptureStderr();
  print_solution(stderr, 3, env);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "SOLUTION: a = 17, b = 23, c = [-1;42], BEST: 77\n");
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_best())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(77));
  testing::internal::CaptureStdout();
  print_solution(stdout, 3, env);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "SOLUTION: a = 17, b = 23, c = [-1;42], BEST: 77\n");
  delete(MockProxy);
}

TEST(PrintError, Basic) {
  std::string output;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, main_name()).Times(1).WillOnce(::testing::Return("<name>"));
  testing::internal::CaptureStderr();
  print_error(ERROR_MSG_NULL_BIND);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "<name>: error: " ERROR_MSG_NULL_BIND "\n");
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, main_name()).Times(1).WillOnce(::testing::Return("<other>"));
  testing::internal::CaptureStderr();
  print_error(ERROR_MSG_OUT_OF_MEMORY);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "<other>: error: " ERROR_MSG_OUT_OF_MEMORY "\n");
  delete(MockProxy);
}

TEST(PrintFatal, Basic) {
  std::string output;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, main_name()).Times(1).WillOnce(::testing::Return("<name>"));
  EXPECT_CALL(*MockProxy, exit(EXIT_FAILURE)).Times(1);
  testing::internal::CaptureStderr();
  print_fatal(ERROR_MSG_OUT_OF_MEMORY);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "<name>: error: " ERROR_MSG_OUT_OF_MEMORY "\n");
  delete(MockProxy);
}

} // end namespace
