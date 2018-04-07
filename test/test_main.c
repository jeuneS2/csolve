#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace _main {
#include "../src/main.c"

class Mock {
 public:
  MOCK_METHOD0(yyparse, int(void));
  MOCK_METHOD1(yyset_in, void(FILE *));
  MOCK_METHOD1(alloc_init, void(size_t));
  MOCK_METHOD1(bind_init, void(size_t));
  MOCK_METHOD1(strategy_prefer_failing_init, void(bool));
  MOCK_METHOD1(strategy_compute_weights_init, void(bool));
  MOCK_METHOD1(strategy_order_init, void(enum order_t));
  MOCK_METHOD2(print_error, void (const char *, va_list));
};

Mock *MockProxy;

int yyparse() {
  return MockProxy->yyparse();
}

void yyset_in(FILE *file) {
  MockProxy->yyset_in(file);
}

void alloc_init(size_t size) {
  MockProxy->alloc_init(size);
}

void bind_init(size_t size) {
  MockProxy->bind_init(size);
}

void strategy_prefer_failing_init(bool prefer_failing) {
  MockProxy->strategy_prefer_failing_init(prefer_failing);
}

void strategy_compute_weights_init(bool compute_weights) {
  MockProxy->strategy_compute_weights_init(compute_weights);
}

void strategy_order_init(enum order_t order) {
  MockProxy->strategy_order_init(order);
}

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

TEST(MainName, Basic) {
  _main_name = "<foo>";
  EXPECT_STREQ("<foo>", main_name());
  _main_name = "<bar>";
  EXPECT_STREQ("<bar>", main_name());
}

TEST(PrintVersion, Basic) {
  std::string output;
  testing::internal::CaptureStdout();
  print_version(stdout);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, VERSION "\n"
            COPYRIGHT "\n"
            "This is free software; see the source for copying conditions.  There is NO\n"
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

TEST(ParseOptions, Version) {
  std::string output;

  _main_name = "<foo>";

  int argc = 2;
  const char *argv [argc] = { "<xxx>", "-v" };

  testing::internal::CaptureStdout();
  EXPECT_EXIT(parse_options(argc, (char **)argv), ::testing::ExitedWithCode(0), "");
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, VERSION "\n"
            COPYRIGHT "\n"
            "This is free software; see the source for copying conditions.  There is NO\n"
            "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

TEST(PrintUsage, Basic) {
  std::string output;

  _main_name = "<foo>";

  testing::internal::CaptureStdout();
  print_usage(stdout);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Usage: <foo> [<options>] [<file>]\n");

  testing::internal::CaptureStderr();
  print_usage(stderr);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "Usage: <foo> [<options>] [<file>]\n");
}

TEST(PrintHelp, Basic) {
  std::string output;

  _main_name = "<bar>";

  testing::internal::CaptureStdout();
  print_help(stdout);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Usage: <bar> [<options>] [<file>]\n"
            "Options:\n"
            "  -b --binds <size>           maximum number of binds (default: 1024)\n"
            "  -h --help                   show this message and exit\n"
            "  -m --memory <size>          allocation stack size in bytes (default: 16777216)\n"
            "  -o --order <order>          how to order variables during solving (default: ORDER_SMALLEST_DOMAIN)\n"
            "  -p --prefer-failing <bool>  prefer failing variables when ordering (default: true)\n"
            "  -v --version                print version and exit\n"
            "  -w --weighten <bool>        compute weights of variables for initial order (default: true)\n");
}

TEST(ParseOptions, Help) {
  std::string output;

  _main_name = "<bar>";

  int argc = 2;
  const char *argv [argc] = { "<xxx>", "-h" };

  testing::internal::CaptureStdout();
  EXPECT_EXIT(parse_options(argc, (char **)argv), ::testing::ExitedWithCode(0), "");
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Usage: <bar> [<options>] [<file>]\n"
            "Options:\n"
            "  -b --binds <size>           maximum number of binds (default: 1024)\n"
            "  -h --help                   show this message and exit\n"
            "  -m --memory <size>          allocation stack size in bytes (default: 16777216)\n"
            "  -o --order <order>          how to order variables during solving (default: ORDER_SMALLEST_DOMAIN)\n"
            "  -p --prefer-failing <bool>  prefer failing variables when ordering (default: true)\n"
            "  -v --version                print version and exit\n"
            "  -w --weighten <bool>        compute weights of variables for initial order (default: true)\n");
}

TEST(ParseOptions, InvalidOption) {
  _main_name = "<xxx>";

  int argc = 2;
  const char *argv [argc] = { "<xxx>", "-X" };
  EXPECT_DEATH(parse_options(argc, (char **)argv),
               "<xxx>: invalid option .*\nUsage: .*\n");
}

TEST(ParseOptions, MoreFiles) {
  _main_name = "<xxx>";

  int argc = 3;
  const char *argv [argc] = { "<xxx>", "X", "Y" };
  EXPECT_DEATH(parse_options(argc, (char **)argv),
               "Usage: .*\n");
}

TEST(ParseOptions, Stdout) {
  _main_name = "<xxx>";

  int argc1 = 1;
  const char *argv1 [argc1] = { "<xxx>" };
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc1, (char **)argv1);
  delete(MockProxy);

  int argc2 = 2;
  const char *argv2 [argc2] = { "<xxx>", "-" };
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc2, (char **)argv2);
  delete(MockProxy);
}

}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
