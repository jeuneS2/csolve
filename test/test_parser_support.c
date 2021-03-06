#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace parser_support {

void mock_free(void *);
#define free mock_free

#include "../src/constr_types.c"
#include "../src/parser_support.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD0(objective_best, domain_t(void));
  MOCK_METHOD2(clause_list_append, void(clause_list_t*, wand_expr_t*));
  MOCK_METHOD2(clause_list_contains, bool(clause_list_t*, wand_expr_t*));
  MOCK_METHOD1(print_fatal, void (const char *));
  MOCK_METHOD2(print_val, void(FILE *, struct val_t));
  MOCK_METHOD1(free, void(void *));
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

void clause_list_append(clause_list_t *list, wand_expr_t *elem) {
  MockProxy->clause_list_append(list, elem);
}

bool clause_list_contains(clause_list_t *list, wand_expr_t *elem) {
  return MockProxy->clause_list_contains(list, elem);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

void print_val(FILE *file, const struct val_t val) {
  MockProxy->print_val(file, val);
  static int i = 0;
  fprintf(file, "<val%i>", i++);
}

void free(void *ptr) {
  MockProxy->free(ptr);
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

TEST(VarsFindKey, Find) {
  struct constr_t val = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct env_t v[2]  = { { "x", &val, NULL, {0, NULL}, 0, 1, 0 },
                         { "y", &val, NULL, {0, NULL}, 1, 0, 0 } };
  _vars = &v[0];
  _var_count = 2;
  keytab_add(0);
  keytab_add(1);

  EXPECT_EQ(&_vars[1], vars_find_key("y"));
  EXPECT_EQ(&_vars[0], vars_find_key("x"));

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("x") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("y") % TABLE_SIZE]));
  keytab_free();
  delete(MockProxy);
}

TEST(VarsFindKey, NotFound) {
  struct constr_t val = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct env_t v[2]  = { { "x", &val, NULL, {0, NULL}, 0, 1, 0 },
                         { "y", &val, NULL, {0, NULL}, 1, 0, 0 } };
  _vars = &v[0];
  _var_count = 2;
  keytab_add(0);
  keytab_add(1);

  EXPECT_EQ(NULL, vars_find_key("z"));

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("x") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("y") % TABLE_SIZE]));
  keytab_free();
  delete(MockProxy);
}

TEST(VarsFindVal, Find) {
  struct constr_t val1 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct constr_t val2 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct env_t v[2]  = { { "x", &val1, NULL, {0, NULL}, 0, 1, 0 },
                         { "y", &val2, NULL, {0, NULL}, 1, 0, 0 } };
  _vars = &v[0];
  _var_count = 2;
  valtab_add(0);
  valtab_add(1);

  EXPECT_EQ(&_vars[1], vars_find_val(&val2));
  EXPECT_EQ(&_vars[0], vars_find_val(&val1));

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&val1) % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&val2) % TABLE_SIZE]));
  valtab_free();
  delete(MockProxy);
}

TEST(VarsFindVal, NotFound) {
  struct constr_t val1 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct constr_t val2 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct constr_t val3 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct env_t v[2]  = { { "x", &val1, NULL, {0, NULL}, 0, 1, 0 },
                         { "y", &val2, NULL, {0, NULL}, 1, 0, 0 } };
  _vars = &v[0];
  _var_count = 2;
  valtab_add(0);
  valtab_add(1);

  EXPECT_EQ(NULL, vars_find_val(&val3));

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&val1) % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&val2) % TABLE_SIZE]));
  valtab_free();
  delete(MockProxy);
}

TEST(VarCount, Basic) {
  _var_count = 11;
  EXPECT_EQ(11U, var_count());
  _var_count = 13;
  EXPECT_EQ(13U, var_count());
}

TEST(VarsAdd, Basic) {
  _vars = NULL;
  _var_count = 0;

  const char *k1 = "k1";
  struct constr_t v1;
  vars_add(k1, &v1);
  EXPECT_NE((struct env_t *)NULL, _vars);
  EXPECT_EQ(1U, _var_count);
  EXPECT_NE(k1, _vars[0].key);
  EXPECT_STREQ(k1, _vars[0].key);
  EXPECT_EQ(&v1, _vars[0].val);
  EXPECT_EQ(0, _vars[0].prio);

  const char *k2 = "k2";
  struct constr_t v2;
  vars_add(k2, &v2);
  EXPECT_NE(k2, _vars[1].key);
  EXPECT_STREQ(k2, _vars[1].key);
  EXPECT_EQ(&v2, _vars[1].val);
  EXPECT_EQ(0, _vars[1].prio);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k1") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k2") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v1) % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v2) % TABLE_SIZE]));
  keytab_free();
  valtab_free();
  delete(MockProxy);
}

TEST(VarsCount, Basic) {
  struct constr_t A = CONSTRAINT_TERM(VALUE(1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(17,  23));
  struct constr_t X;
  struct constr_t Y;

  X = CONSTRAINT_EXPR(EQ, &A, &A);
  EXPECT_EQ(0, vars_count(&X));
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(1, vars_count(&X));
  X = CONSTRAINT_EXPR(ADD, &B, &B);
  EXPECT_EQ(2, vars_count(&X));
  X = CONSTRAINT_EXPR(MUL, &B, &B);
  Y = CONSTRAINT_EXPR(AND, &X, &B);
  EXPECT_EQ(3, vars_count(&Y));
  X = CONSTRAINT_EXPR(OR, &B, &B);
  Y = CONSTRAINT_EXPR(EQ, &X, &X);
  EXPECT_EQ(4, vars_count(&Y));

  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(0, vars_count(&X));
  X = CONSTRAINT_EXPR(NOT, &B, NULL);
  EXPECT_EQ(1, vars_count(&X));
  X = CONSTRAINT_EXPR(EQ, &B, &B);
  Y = CONSTRAINT_EXPR(NEG, &X, NULL);
  EXPECT_EQ(2, vars_count(&Y));
}

TEST(VarsCount, Errors) {
  struct constr_type_t CONSTR_FOO = { NULL, NULL, NULL, (enum operator_t)-1 };
  struct constr_t X;
  std::string output;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(FOO, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  vars_count(&X);
  delete(MockProxy);
}

TEST(VarsWeighten, Basic) {
  struct constr_t X = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct constr_t Y = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct env_t v[2]  = { { "x", &X, NULL, {0, NULL}, 0, 0, 0 },
                         { "y", &Y, NULL, {0, NULL}, 1, 3, 0 } };
  _vars = &v[0];
  _var_count = 2;
  valtab_add(0);
  valtab_add(1);

  struct constr_t Z;

  Z = CONSTRAINT_EXPR(EQ, &X, &Y);
  vars_weighten(&Z, 10);
  EXPECT_EQ(v[0].prio, 10);
  EXPECT_EQ(v[1].prio, 13);

  Z = CONSTRAINT_EXPR(AND, &X, &Y);
  vars_weighten(&Z, 15);
  EXPECT_EQ(v[0].prio, 25);
  EXPECT_EQ(v[1].prio, 28);

  Z = CONSTRAINT_EXPR(NEG, &X, NULL);
  vars_weighten(&Z, 100);
  EXPECT_EQ(v[0].prio, 125);
  EXPECT_EQ(v[1].prio, 28);

  Z = CONSTRAINT_EXPR(NOT, &X, NULL);
  vars_weighten(&Z, 200);
  EXPECT_EQ(v[0].prio, 325);
  EXPECT_EQ(v[1].prio, 28);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&X) % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&Y) % TABLE_SIZE]));
  valtab_free();
  delete(MockProxy);
}

TEST(VarsWeighten, Errors) {
  struct constr_type_t CONSTR_FOO = { NULL, NULL, NULL, (enum operator_t)-1 };
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(FOO, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  vars_weighten(&X, 1);
  delete(MockProxy);
}

TEST(EnvGenerate, Basic) {
  _vars = 0;
  _var_count = 0;
  struct constr_t v1;
  vars_add("k1", &v1);
  struct constr_t v2;
  vars_add("k2", &v2);

  MockProxy = new Mock();
  struct env_t *env = env_generate();
  EXPECT_EQ(env, _vars);
  EXPECT_STREQ("k1", env[0].key);
  EXPECT_EQ(&v1, env[0].val);
  EXPECT_EQ(0, env[0].prio);
  EXPECT_STREQ("k2", env[1].key);
  EXPECT_EQ(&v2, env[1].val);
  EXPECT_EQ(0, env[1].prio);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k1") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k2") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v1) % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v2) % TABLE_SIZE]));
  keytab_free();
  valtab_free();
  delete(MockProxy);
}

TEST(EnvGenerate, Unbounded) {
  _vars = 0;
  _var_count = 0;
  struct constr_t v1 = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, 0));
  vars_add("k1", &v1);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_UNBOUNDED_VARIABLE)).Times(1);
  env_generate();
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k1") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v1) % TABLE_SIZE]));
  keytab_free();
  valtab_free();
  delete(MockProxy);

  _vars = 0;
  _var_count = 0;
  struct constr_t v2 = CONSTRAINT_TERM(INTERVAL(0, DOMAIN_MAX));
  vars_add("k2", &v2);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_UNBOUNDED_VARIABLE)).Times(1);
  env_generate();
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free(_keytab[hash_str("k2") % TABLE_SIZE]));
  EXPECT_CALL(*MockProxy, free(_valtab[hash_val(&v2) % TABLE_SIZE]));
  keytab_free();
  valtab_free();
  delete(MockProxy);
}

TEST(EnvFree, Basic) {
  struct wand_expr_t w;
  struct wand_expr_t *x [1] = { &w };
  struct wand_expr_t *y [1] = { &w };
  struct env_t env[2];

  struct constr_t a = CONSTRAINT_TERM(INTERVAL(1, 27));
  env[0] = { .key = "a", .val = &a, .binds = NULL,
             .clauses = { .length = 0, .elems = x },
             .order = 0, .prio = 3, .level = 0 };
  struct constr_t b = CONSTRAINT_TERM(INTERVAL(3, 17));
  env[1] = { .key = "b", .val = &b, .binds = NULL,
             .clauses = { .length = 0, .elems = y },
             .order = 0, .prio = 4, .level = 0 };
  _vars = env;
  _var_count = 2;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, free((void *)env[0].key));
  EXPECT_CALL(*MockProxy, free((void *)env[1].key));
  EXPECT_CALL(*MockProxy, free((void *)env[0].clauses.elems));
  EXPECT_CALL(*MockProxy, free((void *)env[1].clauses.elems));
  EXPECT_CALL(*MockProxy, free((void *)_vars));
  env_free();
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

TEST(ClausesInit, Term) {
  struct constr_t e1 = CONSTRAINT_TERM(VALUE(1));
  struct constr_t e2 = CONSTRAINT_TERM(INTERVAL(0, 1));

  struct env_t v[2]  = { { "x", &e1, NULL, {0, NULL}, 0, 0, 0 },
                         { "y", &e2, NULL, {0, NULL}, 1, 3, 0 } };
  e1.constr.term.env = &v[0];
  e2.constr.term.env = &v[1];

  struct wand_expr_t w;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, clause_list_contains(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*MockProxy, clause_list_append(testing::_, testing::_)).Times(0);
  clauses_init(&e1, NULL);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, clause_list_contains(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*MockProxy, clause_list_append(testing::_, testing::_)).Times(0);
  clauses_init(&e1, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, clause_list_contains(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*MockProxy, clause_list_append(testing::_, testing::_)).Times(0);
  clauses_init(&e2, NULL);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&e2, &w);
  delete(MockProxy);
}

TEST(ClausesInit, Wand) {
  struct constr_t e1 = CONSTRAINT_TERM(VALUE(11));
  struct constr_t e2 = CONSTRAINT_TERM(INTERVAL(0,1));
  struct env_t v[2]  = { { "x", &e1, NULL, {0, NULL}, 0, 0, 0 },
                         { "y", &e2, NULL, {0, NULL}, 1, 3, 0 } };
  e1.constr.term.env = &v[0];
  e2.constr.term.env = &v[1];

  struct wand_expr_t E [2] = { { .constr = &e1, .orig = &e1, .prop_tag = 0 }, { .constr = &e2, .orig = &e2, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  struct wand_expr_t F [2] = { { .constr = &e1, .orig = &e1, .prop_tag = 0 }, { .constr = &X, .orig = &X, .prop_tag = 0 } };
  struct constr_t Y = CONSTRAINT_WAND(2, F);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &E[1])).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &E[1])).Times(1);
  clauses_init(&Y, NULL);
  delete(MockProxy);
}

TEST(ClausesInit, Expr) {
  struct constr_type_t CONSTR_FOO = { NULL, NULL, NULL, (enum operator_t)-1 };

  struct constr_t e1 = CONSTRAINT_TERM(INTERVAL(2, 3));
  struct constr_t e2 = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t e3 = CONSTRAINT_TERM(VALUE(1));

  struct env_t v[2]  = { { "x", &e1, NULL, {0, NULL}, 0, 0, 0 },
                         { "y", &e2, NULL, {0, NULL}, 1, 3, 0 } };
  e1.constr.term.env = &v[0];
  e2.constr.term.env = &v[1];

  struct constr_t X;

  struct wand_expr_t w;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*MockProxy, clause_list_append(testing::_, testing::_)).Times(0);
  clauses_init(&X, NULL);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &e3, &e3);
  EXPECT_CALL(*MockProxy, clause_list_contains(testing::_, testing::_)).Times(0);
  EXPECT_CALL(*MockProxy, clause_list_append(testing::_, testing::_)).Times(0);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &e1, &e2);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[1].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[1].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &e1, NULL);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &e1, NULL);
  EXPECT_CALL(*MockProxy, clause_list_contains(&v[0].clauses, &w)).Times(1).WillRepeatedly(::testing::Return(false));
  EXPECT_CALL(*MockProxy, clause_list_append(&v[0].clauses, &w)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(FOO, NULL, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  clauses_init(&X, &w);
  delete(MockProxy);
}

}
