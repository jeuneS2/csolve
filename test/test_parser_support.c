#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdarg.h>

namespace parser_support {

void mock_free(void *);
void mock_qsort(void *, size_t, size_t, int (*)(const void *, const void *));
#define free mock_free
#define qsort mock_qsort

#include "../src/parser_support.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD2(print_fatal, void (const char *, va_list));
  MOCK_METHOD2(print_val, void(FILE *, struct val_t));
  MOCK_METHOD1(free, void(void *));
  MOCK_METHOD4(qsort, void(void *, size_t, size_t, int (*)(const void *, const void *)));
};

Mock *MockProxy;

domain_t objective_best(void) {
  return MockProxy->objective_best();
}

void print_fatal(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_fatal(fmt, args);
  va_end(args);
}

void print_val(FILE *file, const struct val_t val) {
  MockProxy->print_val(file, val);
  static int i = 0;
  fprintf(file, "<val%i>", i++);
}

void free(void *ptr) {
  MockProxy->free(ptr);
}

void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *)) {
  MockProxy->qsort(base, nmemb, size, compar);
}

TEST(VarsFindKey, Find) {
  struct val_t val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val }, 0 },
                         { { "y", &val }, 1 } };
  _vars = &v[0];
  _var_count = 2;

  EXPECT_EQ(&_vars[1], vars_find_key("y"));
  EXPECT_EQ(&_vars[0], vars_find_key("x"));
}

TEST(VarsFindKey, NotFound) {
  struct val_t val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val }, 0 },
                         { { "y", &val }, 1 } };
  _vars = &v[0];
  _var_count = 2;

  EXPECT_EQ(NULL, vars_find_key("z"));
}

TEST(VarsFindVal, Find) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 1 } };
  _vars = &v[0];
  _var_count = 2;

  EXPECT_EQ(&_vars[1], vars_find_val(&val2));
  EXPECT_EQ(&_vars[0], vars_find_val(&val1));
}

TEST(VarsFindVal, NotFound) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val3 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 1 } };
  _vars = &v[0];
  _var_count = 2;

  EXPECT_EQ(NULL, vars_find_val(&val3));
}

TEST(VarsAdd, Basic) {
  _vars = NULL;
  _var_count = 0;

  const char *k1 = "k1";
  struct val_t v1;
  vars_add(k1, &v1);
  EXPECT_NE((struct var_t *)NULL, _vars);
  EXPECT_EQ(1, _var_count);
  EXPECT_NE(k1, _vars[0].var.key);
  EXPECT_STREQ(k1, _vars[0].var.key);
  EXPECT_EQ(&v1, _vars[0].var.val);
  EXPECT_EQ(0, _vars[0].var.fails);
  EXPECT_NE((struct solve_step_t *)NULL, _vars[0].var.step);
  EXPECT_EQ(0, _vars[0].weight);

  const char *k2 = "k2";
  struct val_t v2;
  vars_add(k2, &v2);
  EXPECT_NE(k2, _vars[1].var.key);
  EXPECT_STREQ(k2, _vars[1].var.key);
  EXPECT_EQ(&v2, _vars[1].var.val);
  EXPECT_EQ(0, _vars[1].var.fails);
  EXPECT_NE((struct solve_step_t *)NULL, _vars[1].var.step);
  EXPECT_EQ(0, _vars[1].weight);
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
  X = CONSTRAINT_EXPR(OP_LT, &A, &B);
  EXPECT_EQ(1, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_ADD, &B, &B);
  EXPECT_EQ(2, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_MUL, &B, &B);
  Y = CONSTRAINT_EXPR(OP_AND, &X, &B);
  EXPECT_EQ(3, vars_count(&Y));
  X = CONSTRAINT_EXPR(OP_OR, &B, &B);
  Y = CONSTRAINT_EXPR(OP_EQ, &X, &X);
  EXPECT_EQ(4, vars_count(&Y));

  X = CONSTRAINT_EXPR(OP_NEG, &A, NULL);
  EXPECT_EQ(0, vars_count(&X));
  X = CONSTRAINT_EXPR(OP_NOT, &B, NULL);
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
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  vars_count(&X);
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, testing::_)).Times(1);
  vars_count(&X);
  delete(MockProxy);
}

TEST(VarsWeighten, Basic) {
  struct val_t val1 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct val_t val2 = INTERVAL(DOMAIN_MIN, DOMAIN_MAX);
  struct var_t v[2]  = { { { "x", &val1 }, 0 },
                         { { "y", &val2 }, 3 } };
  _vars = &v[0];
  _var_count = 2;

  struct constr_t X = { .type = CONSTR_TERM, .constr = { .term = &val1 } };
  struct constr_t Y = { .type = CONSTR_TERM, .constr = { .term = &val2 } };
  struct constr_t Z;

  Z = CONSTRAINT_EXPR(OP_EQ, &X, &Y);
  vars_weighten(&Z, 10);
  EXPECT_EQ(v[0].weight, 10);
  EXPECT_EQ(v[1].weight, 13);

  Z = CONSTRAINT_EXPR(OP_AND, &X, &Y);
  vars_weighten(&Z, 15);
  EXPECT_EQ(v[0].weight, 25);
  EXPECT_EQ(v[1].weight, 28);

  Z = CONSTRAINT_EXPR(OP_NEG, &X, NULL);
  vars_weighten(&Z, 100);
  EXPECT_EQ(v[0].weight, 125);
  EXPECT_EQ(v[1].weight, 28);

  Z = CONSTRAINT_EXPR(OP_NOT, &X, NULL);
  vars_weighten(&Z, 200);
  EXPECT_EQ(v[0].weight, 325);
  EXPECT_EQ(v[1].weight, 28);
}

TEST(VarsWeighten, Errors) {
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR((enum operator_t)-1, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION, testing::_)).Times(1);
  vars_weighten(&X, 1);
  delete(MockProxy);

  MockProxy = new Mock();
  X = { .type = (enum constr_type_t)-1, .constr = { .term = NULL } };
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_CONSTRAINT_TYPE, testing::_)).Times(1);
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
  _vars = &v[0];
  _var_count = 2;

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

TEST(VarsFree, Basic) {
  _vars = 0;
  _var_count = 0;

  struct val_t val;
  vars_add("key", &val);
  EXPECT_NE((struct var_t *)NULL, _vars);
  EXPECT_EQ(1, _var_count);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_vars)).Times(1);
  vars_free();
  EXPECT_EQ((struct var_t *)NULL, _vars);
  EXPECT_EQ(0, _var_count);
  delete(MockProxy);
}

TEST(VarsSort, Basic) {
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, qsort(_vars, _var_count, sizeof(struct var_t), vars_compare)).Times(1);
  vars_sort();
  delete(MockProxy);
}

TEST(EnvGenerate, Basic) {
  _vars = 0;
  _var_count = 0;
  struct val_t v1;
  vars_add("k1", &v1);
  struct val_t v2;
  vars_add("k2", &v2);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_vars)).Times(1);
  struct env_t *env = env_generate();
  EXPECT_STREQ("k1", env[0].key);
  EXPECT_EQ(&v1, env[0].val);
  EXPECT_EQ(0, env[0].fails);
  EXPECT_NE((struct solve_step_t *)NULL, env[0].step);
  EXPECT_STREQ("k2", env[1].key);
  EXPECT_EQ(&v2, env[1].val);
  EXPECT_EQ(0, env[1].fails);
  EXPECT_NE((struct solve_step_t *)NULL, env[1].step);
  EXPECT_EQ((const char *)NULL, env[2].key);
  EXPECT_EQ((struct val_t *)NULL, env[2].val);
  EXPECT_EQ(0, env[2].fails);
  EXPECT_NE((struct solve_step_t *)NULL, env[1].step);
  EXPECT_EQ((struct var_t *)NULL, _vars);
  EXPECT_EQ(0, _var_count);
  delete(MockProxy);
}

TEST(EnvFree, Basic) {
  struct env_t env[3];

  struct val_t a = INTERVAL(1, 27);
  struct solve_step_t sA;
  env[0] = { .key = "a", .val = &a, .fails = 3, .step = &sA };
  struct val_t b = INTERVAL(3, 17);
  struct solve_step_t sB;
  env[1] = { .key = "b", .val = &b, .fails = 4, .step = &sB };
  env[2] = { .key = NULL, .val = NULL, .fails = 0, .step = NULL };

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free((void *)env[0].key));
  EXPECT_CALL(*MockProxy, free((void *)env[0].step));
  EXPECT_CALL(*MockProxy, free((void *)env[1].key));
  EXPECT_CALL(*MockProxy, free((void *)env[1].step));
  EXPECT_CALL(*MockProxy, free((void *)env[2].step));
  EXPECT_CALL(*MockProxy, free((void *)&env[0]));
  env_free(env);
  delete(MockProxy);
}

TEST(ExprListAppend, Basic) {
  struct constr_t e1;
  struct expr_list_t *l1 = expr_list_append(NULL, &e1);
  EXPECT_NE((struct expr_list_t *)NULL, l1);
  EXPECT_EQ((struct expr_list_t *)NULL, l1->next);
  EXPECT_EQ(&e1, l1->expr);

  struct constr_t e2;
  struct expr_list_t *l2 = expr_list_append(l1, &e2);
  EXPECT_NE((struct expr_list_t *)NULL, l2);
  EXPECT_EQ(l1, l2->next);
  EXPECT_EQ(&e2, l2->expr);
}

TEST(ExprListFree, Basic) {
  struct constr_t e1;
  struct expr_list_t *l1 = expr_list_append(NULL, &e1);
  struct constr_t e2;
  struct expr_list_t *l2 = expr_list_append(l1, &e2);
  struct constr_t e3;
  struct expr_list_t *l3 = expr_list_append(l2, &e3);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(l1)).Times(1);
  EXPECT_CALL(*MockProxy, free(l2)).Times(1);
  EXPECT_CALL(*MockProxy, free(l3)).Times(1);
  expr_list_free(l3);
  delete(MockProxy);
}

}
