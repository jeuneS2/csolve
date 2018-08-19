#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace print {

void mock_exit(int code);
#define exit mock_exit

#include "../src/print.c"

class Mock {
 public:
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD0(main_name, const char *(void));
  MOCK_METHOD1(exit, void(int));
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
  struct val_t a = VALUE(17);
  struct val_t b = INTERVAL(23,42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
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

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (= 17 [23;42])");

  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  testing::internal::CaptureStdout();
  print_constr(stdout, &X);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, " (= 17 [23;42])");

  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (< 17 [23;42])");

  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (- 17)");

  X = CONSTRAINT_EXPR(OP_ADD, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (+ [23;42] 17)");

  X = CONSTRAINT_EXPR(OP_MUL, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (* [23;42] 17)");

  X = CONSTRAINT_EXPR(OP_AND, &A, &B);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (& 17 [23;42])");

  X = CONSTRAINT_EXPR(OP_OR, &B, &A);
  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " (| [23;42] 17)");
}

TEST(PrintConstr, Wand) {
  std::string output;

  struct val_t a = VALUE(17);
  struct val_t b = INTERVAL(23,42);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct wand_expr_t E [2] = { { .constr = &A, .prop_tag = 0 }, { .constr = &B, .prop_tag = 0 } };
  struct constr_t X = { .type = CONSTR_WAND, .constr = { .wand = { .length = 2, .elems = E } } };

  testing::internal::CaptureStderr();
  print_constr(stderr, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, " 17; [23;42];");
}

TEST(PrintConstr, Error) {
  std::string output;
  struct constr_t X;

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, main_name()).Times(1).WillOnce(::testing::Return("<name>"));
  testing::internal::CaptureStderr();
  print_constr(stdout, &X);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "<name>: error: invalid constraint type: ffffffff\n");
  delete(MockProxy);
}

TEST(PrintEnv, Basic) {
  std::string output;
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = INTERVAL(-1,42);

  struct env_t env [4] = { { "a", &a },
                           { "b", &b },
                           { "c", &c },
                           { NULL, NULL } };

  testing::internal::CaptureStderr();
  print_env(stderr, env);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "a = 17, b = 23, c = [-1;42], ");

  testing::internal::CaptureStdout();
  print_env(stdout, env);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "a = 17, b = 23, c = [-1;42], ");
}

TEST(PrintSolution, Basic) {
  std::string output;
  struct val_t a = VALUE(17);
  struct val_t b = VALUE(23);
  struct val_t c = INTERVAL(-1,42);

  struct env_t env [4] = { { "a", &a },
                           { "b", &b },
                           { "c", &c },
                           { NULL, NULL } };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_best())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(77));
  testing::internal::CaptureStderr();
  print_solution(stderr, env);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "SOLUTION: a = 17, b = 23, c = [-1;42], BEST: 77\n");
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, objective_best())
    .Times(::testing::AtLeast(1))
    .WillRepeatedly(::testing::Return(77));
  testing::internal::CaptureStdout();
  print_solution(stdout, env);
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
