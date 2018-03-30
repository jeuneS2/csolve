#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace parser_support {
#include "../src/parser_support.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD2(print_error, void (const char *, va_list));
  MOCK_METHOD2(print_val, void(FILE *, struct val_t));
};

Mock *MockProxy;

domain_t objective_best(void) {
  return MockProxy->objective_best();
}

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

void print_val(FILE *file, const struct val_t val) {
  MockProxy->print_val(file, val);
  static int i = 0;
  fprintf(file, "<val%i>", i++);
}

TEST(VarsFindKey, Find) {
  struct val_t val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val }, 0 },
                         { { "y", &val }, 1 } };
  vars = &v[0];
  var_count = 2;

  EXPECT_EQ(&vars[1], vars_find_key("y"));
  EXPECT_EQ(&vars[0], vars_find_key("x"));
}

TEST(VarsFindKey, NotFound) {
  struct val_t val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val }, 0 },
                         { { "y", &val }, 1 } };
  vars = &v[0];
  var_count = 2;

  EXPECT_EQ(NULL, vars_find_key("z"));
}

TEST(VarsFindVal, Find) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 1 } };
  vars = &v[0];
  var_count = 2;

  EXPECT_EQ(&vars[1], vars_find_val(&val2));
  EXPECT_EQ(&vars[0], vars_find_val(&val1));
}

TEST(VarsFindVal, NotFound) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val3 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 1 } };
  vars = &v[0];
  var_count = 2;

  EXPECT_EQ(NULL, vars_find_val(&val3));
}

TEST(VarsCount, Basic) {
  struct val_t a = VALUE(1);
  struct val_t b = INTERVAL(17, 23);

  struct constr_t A = { .type = CONSTR_TERM, .constr = { .term = &a } };
  struct constr_t B = { .type = CONSTR_TERM, .constr = { .term = &b } };
  struct constr_t X;
  struct constr_t Y;

  X = CONSTRAINT_EXPR(OP_EQ, &A, &A);
  EXPECT_EQ(0, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_EQ, &A, &B);
  EXPECT_EQ(1, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_EQ, &B, &B);
  EXPECT_EQ(2, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_EQ, &B, &B);
  Y = CONSTRAINT_EXPR(OP_EQ, &X, &B);
  EXPECT_EQ(3, vars_count(&Y));
  X = CONSTRAINT_EXPR(OP_EQ, &B, &B);
  Y = CONSTRAINT_EXPR(OP_EQ, &X, &X);
  EXPECT_EQ(4, vars_count(&Y));

  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(0, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_NEG, &B, NULL);
  EXPECT_EQ(1, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_EQ, &B, &B);
  Y = CONSTRAINT_EXPR(OP_NEG, &X, NULL);
  EXPECT_EQ(2, vars_count(&Y));
}

TEST(VarsCount, Errors) {
  struct constr_t X;
  std::string output;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  EXPECT_EQ(0, vars_count(&X));
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_CONSTRAINT_TYPE, testing::_)).Times(1);
  EXPECT_EQ(0, vars_count(&X));
  delete(MockProxy);
}

TEST(VarsWeighten, Basic) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 3 } };
  vars = &v[0];
  var_count = 2;

  struct constr_t X = { .type = CONSTR_TERM, .constr = { .term = &val1 } };
  struct constr_t Y = { .type = CONSTR_TERM, .constr = { .term = &val2 } };
  struct constr_t Z;

  Z = CONSTRAINT_EXPR(OP_EQ, &X, &Y);
  vars_weighten(&Z, 10);
  EXPECT_EQ(v[0].weight, 10);
  EXPECT_EQ(v[1].weight, 13);

  Z = CONSTRAINT_EXPR(OP_NEG, &X, NULL);
  vars_weighten(&Z, 100);
  EXPECT_EQ(v[0].weight, 110);
  EXPECT_EQ(v[1].weight, 13);
}

TEST(VarsWeighten, Errors) {
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  vars_weighten(&X, 1);
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_INVALID_CONSTRAINT_TYPE, testing::_)).Times(1);
  vars_weighten(&X, 1);
  delete(MockProxy);
}

TEST(VarsCompare, Basic) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val3 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t x = { { "x", &val1 }, 10 };
  struct var_t y = { { "y", &val2 }, 20 };
  struct var_t z = { { "z", &val3 }, 20 };

  EXPECT_GT(vars_compare(&x, &y), 0);
  EXPECT_GT(vars_compare(&x, &z), 0);
  EXPECT_LT(vars_compare(&y, &x), 0);
  EXPECT_LT(vars_compare(&z, &x), 0);

  EXPECT_EQ(vars_compare(&y, &y), 0);
  EXPECT_EQ(vars_compare(&y, &z), 0);
  EXPECT_EQ(vars_compare(&z, &y), 0);
}

TEST(VarsPrint, Basic) {
  struct val_t val1 = INTERVAL(-1, 5);
  struct val_t val2 = INTERVAL(18, 86);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 3 } };
  vars = &v[0];
  var_count = 2;

  std::string output;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_val(stderr, val1)).Times(1);
  EXPECT_CALL(*MockProxy, print_val(stderr, val2)).Times(1);
  testing::internal::CaptureStderr();
  vars_print(stderr);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "x: 0<val0>\ny: 3<val1>\n");
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_val(stdout, val1)).Times(1);
  EXPECT_CALL(*MockProxy, print_val(stdout, val2)).Times(1);
  testing::internal::CaptureStdout();
  vars_print(stdout);
  output = testing::internal::GetCapturedStdout();
  EXPECT_EQ(output, "x: 0<val2>\ny: 3<val3>\n");
  delete(MockProxy);
}

}
