#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace _main {
#include "../src/main.c"

class Mock {
 public:
  MOCK_METHOD0(yyparse, int(void));
  MOCK_METHOD0(yyget_in, FILE *(void));
  MOCK_METHOD1(yyset_in, void(FILE *));
  MOCK_METHOD0(yylex_destroy, int(void));
  MOCK_METHOD1(alloc_init, void(size_t));
  MOCK_METHOD0(alloc_free, void(void));
  MOCK_METHOD1(bind_init, void(size_t));
  MOCK_METHOD0(bind_free, void(void));
  MOCK_METHOD1(patch_init, void(size_t));
  MOCK_METHOD0(patch_free, void(void));
  MOCK_METHOD1(shared_init, void(uint32_t));
  MOCK_METHOD1(timeout_init, void(uint32_t));
  MOCK_METHOD1(conflict_alloc_init, void(size_t));
  MOCK_METHOD0(conflict_alloc_free, void(void));
  MOCK_METHOD1(strategy_create_conflicts_init, void(bool));
  MOCK_METHOD1(strategy_prefer_failing_init, void(bool));
  MOCK_METHOD1(strategy_compute_weights_init, void(bool));
  MOCK_METHOD1(strategy_restart_frequency_init, void(uint64_t));
  MOCK_METHOD1(strategy_order_init, void(enum order_t));
  MOCK_METHOD0(strategy_var_order_free, void(void));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

int yyparse() {
  return MockProxy->yyparse();
}

FILE *yyget_in(void) {
  return MockProxy->yyget_in();
}

void yyset_in(FILE *file) {
  MockProxy->yyset_in(file);
}

int yylex_destroy() {
  return MockProxy->yylex_destroy();
}

void alloc_init(size_t size) {
  MockProxy->alloc_init(size);
}

void alloc_free(void) {
  MockProxy->alloc_free();
}

void bind_init(size_t size) {
  MockProxy->bind_init(size);
}

void bind_free(void) {
  MockProxy->bind_free();
}

void patch_init(size_t size) {
  MockProxy->patch_init(size);
}

void patch_free(void) {
  MockProxy->patch_free();
}

void shared_init(uint32_t workers) {
  MockProxy->shared_init(workers);
}

void timeout_init(uint32_t time) {
  MockProxy->timeout_init(time);
}

void conflict_alloc_init(size_t size) {
  MockProxy->conflict_alloc_init(size);
}

void conflict_alloc_free(void) {
  MockProxy->conflict_alloc_free();
}

void strategy_create_conflicts_init(bool create_conflicts) {
  MockProxy->strategy_create_conflicts_init(create_conflicts);
}

 void strategy_prefer_failing_init(bool prefer_failing) {
  MockProxy->strategy_prefer_failing_init(prefer_failing);
}

void strategy_compute_weights_init(bool compute_weights) {
  MockProxy->strategy_compute_weights_init(compute_weights);
}

void strategy_restart_frequency_init(uint64_t restart_frequency) {
  MockProxy->strategy_restart_frequency_init(restart_frequency);
}

void strategy_order_init(enum order_t order) {
  MockProxy->strategy_order_init(order);
}

void strategy_var_order_free(void) {
  MockProxy->strategy_var_order_free();
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
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
  optind = 1;

  testing::internal::CaptureStdout();
  EXPECT_EXIT(parse_options(argc, (char **)argv), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
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
            "  -b --binds <size>           maximum number of binds (default: " + std::to_string(BIND_STACK_SIZE_DEFAULT) + ")\n"
            "  -c --conflicts <bool>       create conflict clauses (default: true)\n"
            "  -f --prefer-failing <bool>  prefer failing variables when ordering (default: true)\n"
            "  -h --help                   show this message and exit\n"
            "  -j --jobs <int>             number of jobs to run simultaneously (default: " + std::to_string(WORKERS_MAX_DEFAULT) + ")\n"
            "  -m --memory <size>          allocation stack size in bytes (default: " + std::to_string(ALLOC_STACK_SIZE_DEFAULT) + ")\n"
            "  -M --confl-memory <size>    conflict allocation stack size in bytes (default: " + std::to_string(CONFLICT_ALLOC_STACK_SIZE_DEFAULT) + ")\n"
            "  -o --order <order>          how to order variables during solving (default: ORDER_NONE)\n"
            "  -p --patches <size>         maximum number of patches (default: " + std::to_string(PATCH_STACK_SIZE_DEFAULT) + ")\n"
            "  -r --restart-freq <int>     restart frequency when looking for any solution (default: " + std::to_string(STRATEGY_RESTART_FREQUENCY_DEFAULT) + "), set to 0 to disable\n"
            "  -t --time <int>             maximum solving time in seconds (default: " + std::to_string(TIME_MAX_DEFAULT) + "), set to 0 to disable\n"
            "  -v --version                print version and exit\n"
            "  -w --weighten <bool>        compute weights of variables for initial order (default: true)\n");
}

TEST(ParseOptions, Help) {
  std::string output;

  _main_name = "<bar>";

  int argc = 2;
  const char *argv [argc] = { "<xxx>", "-h" };
  optind = 1;

  testing::internal::CaptureStdout();
  EXPECT_EXIT(parse_options(argc, (char **)argv), ::testing::ExitedWithCode(EXIT_SUCCESS), "");
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "Usage: <bar> [<options>] [<file>]\n"
            "Options:\n"
            "  -b --binds <size>           maximum number of binds (default: " + std::to_string(BIND_STACK_SIZE_DEFAULT) + ")\n"
            "  -c --conflicts <bool>       create conflict clauses (default: true)\n"
            "  -f --prefer-failing <bool>  prefer failing variables when ordering (default: true)\n"
            "  -h --help                   show this message and exit\n"
            "  -j --jobs <int>             number of jobs to run simultaneously (default: " + std::to_string(WORKERS_MAX_DEFAULT) + ")\n"
            "  -m --memory <size>          allocation stack size in bytes (default: " + std::to_string(ALLOC_STACK_SIZE_DEFAULT) + ")\n"
            "  -M --confl-memory <size>    conflict allocation stack size in bytes (default: " + std::to_string(CONFLICT_ALLOC_STACK_SIZE_DEFAULT) + ")\n"
            "  -o --order <order>          how to order variables during solving (default: ORDER_NONE)\n"
            "  -p --patches <size>         maximum number of patches (default: " + std::to_string(PATCH_STACK_SIZE_DEFAULT) + ")\n"
            "  -r --restart-freq <int>     restart frequency when looking for any solution (default: " + std::to_string(STRATEGY_RESTART_FREQUENCY_DEFAULT) + "), set to 0 to disable\n"
            "  -t --time <int>             maximum solving time in seconds (default: " + std::to_string(TIME_MAX_DEFAULT) + "), set to 0 to disable\n"
            "  -v --version                print version and exit\n"
            "  -w --weighten <bool>        compute weights of variables for initial order (default: true)\n");
}

TEST(ParseOptions, InvalidOption) {
  _main_name = "<xxx>";

  int argc = 2;
  const char *argv [argc] = { "<xxx>", "-X" };
  optind = 1;
  EXPECT_DEATH(parse_options(argc, (char **)argv),
               ".*: .* option .*\nUsage: .*\n");
}

TEST(ParseOptions, MoreFiles) {
  _main_name = "<xxx>";

  int argc = 3;
  const char *argv [argc] = { "<xxx>", "X", "Y" };
  optind = 1;
  EXPECT_DEATH(parse_options(argc, (char **)argv),
               "Usage: .*\n");
}

TEST(ParseOptions, Stdout) {
  _main_name = "<xxx>";

  int argc1 = 1;
  const char *argv1 [argc1] = { "<xxx>" };
  optind = 1;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc1, (char **)argv1);
  delete(MockProxy);

  int argc2 = 2;
  const char *argv2 [argc2] = { "<xxx>", "-" };
  optind = 1;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc2, (char **)argv2);
  delete(MockProxy);
}

TEST(ParseOptions, NonExistentFile) {
  int argc = 2;
  const char *argv [argc] = { "<xxx>", "file_does_not_exist" };
  optind = 1;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, print_fatal("%s: %s")).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, Bind) {
  int argc = 3;
  const char *argv [argc] = { "<xxx>", "-b", "1234" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(1234)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, Alloc) {
  int argc = 3;
  const char *argv [argc] = { "<xxx>", "-m", "1234" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(1234)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, Jobs) {
  int argc = 3;
  const char *argv [argc] = { "<xxx>", "-j", "7" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(7)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, Order) {
  int argc = 3;
  const char *argv [argc] = { "<xxx>", "-o", "largest-value" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(ORDER_LARGEST_VALUE)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, PreferFailing) {
  int argc1 = 3;
  const char *argv1 [argc1] = { "<xxx>", "-f", "false" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(false)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc1, (char **)argv1);
  delete(MockProxy);

  int argc2 = 3;
  const char *argv2 [argc2] = { "<xxx>", "-f", "true" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(true)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc2, (char **)argv2);
  delete(MockProxy);
}

TEST(ParseOptions, Restarts) {
  int argc = 3;
  const char *argv [argc] = { "<xxx>", "-r", "1234" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(1234)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc, (char **)argv);
  delete(MockProxy);
}

TEST(ParseOptions, Weighten) {
  int argc1 = 3;
  const char *argv1 [argc1] = { "<xxx>", "-w", "false" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(false)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc1, (char **)argv1);
  delete(MockProxy);

  int argc2 = 3;
  const char *argv2 [argc2] = { "<xxx>", "-w", "true" };
  optind = 1;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_CREATE_CONFLICTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(true)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  parse_options(argc2, (char **)argv2);
  delete(MockProxy);
}

TEST(ParseBool, Basic) {
  EXPECT_EQ(true, parse_bool("true"));
  EXPECT_EQ(false, parse_bool("false"));
}

TEST(ParseBool, Error) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_BOOL_ARG)).Times(1);
  parse_bool("abc");
  delete(MockProxy);
}

TEST(ParseInt, Basic) {
  EXPECT_EQ(10, parse_int("10"));
  EXPECT_EQ(0x10, parse_int("0x10"));
  EXPECT_EQ(0123, parse_int("0123"));
}

TEST(ParseInt, Error) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_INT_ARG)).Times(1);
  parse_int("abc");
  delete(MockProxy);
}

TEST(ParseOrder, Basic) {
  EXPECT_EQ(ORDER_NONE, parse_order("none"));
  EXPECT_EQ(ORDER_SMALLEST_DOMAIN, parse_order("smallest-domain"));
  EXPECT_EQ(ORDER_LARGEST_DOMAIN, parse_order("largest-domain"));
  EXPECT_EQ(ORDER_SMALLEST_VALUE, parse_order("smallest-value"));
  EXPECT_EQ(ORDER_LARGEST_VALUE, parse_order("largest-value"));
}

TEST(ParseOrder, Error) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_ORDER_ARG)).Times(1);
  parse_order("abc");
  delete(MockProxy);
}

TEST(ParseSize, Basic) {
  EXPECT_EQ(7, parse_size("7"));
  EXPECT_EQ(10*1024, parse_size("10k"));
  EXPECT_EQ(12*1024, parse_size("12K"));
  EXPECT_EQ(0x10*1024*1024, parse_size("0x10m"));
  EXPECT_EQ(0x11*1024*1024, parse_size("0x11M"));
  EXPECT_EQ(02ULL*1024*1024*1024, parse_size("02g"));
  EXPECT_EQ(03ULL*1024*1024*1024, parse_size("03G"));
}

TEST(ParseSize, Error) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_SIZE_ARG)).Times(1);
  parse_size("10kx");
  delete(MockProxy);
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_SIZE_ARG)).Times(1);
  parse_size("13Mx");
  delete(MockProxy);
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_SIZE_ARG)).Times(1);
  parse_size("7Gx");
  delete(MockProxy);
}

TEST(Cleanup, Basic) {
  char *c;
  size_t s;
  FILE *f = open_memstream(&c, &s);
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_free()).Times(1);
  EXPECT_CALL(*MockProxy, patch_free()).Times(1);
  EXPECT_CALL(*MockProxy, alloc_free()).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_free()).Times(1);
  EXPECT_CALL(*MockProxy, strategy_var_order_free()).Times(1);
  EXPECT_CALL(*MockProxy, yyget_in()).Times(1).WillRepeatedly(::testing::Return(f));
  EXPECT_CALL(*MockProxy, yylex_destroy()).Times(1);
  cleanup();
  delete(MockProxy);
}

TEST(Main, Basic) {
  _main_name = "<xxx>";

  char *c;
  size_t s;
  FILE *f = open_memstream(&c, &s);

  int argc = 1;
  const char *argv [argc] = { "<xxx>" };
  optind = 1;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_init(BIND_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, patch_init(PATCH_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, alloc_init(ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, shared_init(WORKERS_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, timeout_init(TIME_MAX_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_init(CONFLICT_ALLOC_STACK_SIZE_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_order_init(STRATEGY_ORDER_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_create_conflicts_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_prefer_failing_init(STRATEGY_PREFER_FAILING_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_compute_weights_init(STRATEGY_COMPUTE_WEIGHTS_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, strategy_restart_frequency_init(STRATEGY_RESTART_FREQUENCY_DEFAULT)).Times(1);
  EXPECT_CALL(*MockProxy, yyset_in(stdin)).Times(1);
  EXPECT_CALL(*MockProxy, yyparse()).Times(1);
  EXPECT_CALL(*MockProxy, bind_free()).Times(1);
  EXPECT_CALL(*MockProxy, patch_free()).Times(1);
  EXPECT_CALL(*MockProxy, alloc_free()).Times(1);
  EXPECT_CALL(*MockProxy, conflict_alloc_free()).Times(1);
  EXPECT_CALL(*MockProxy, strategy_var_order_free()).Times(1);
  EXPECT_CALL(*MockProxy, yyget_in()).Times(1).WillRepeatedly(::testing::Return(f));
  EXPECT_CALL(*MockProxy, yylex_destroy()).Times(1);
  EXPECT_EQ(EXIT_SUCCESS, main(argc, (char **)argv));
  EXPECT_STREQ("<xxx>", _main_name);
  delete(MockProxy);
}

}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
