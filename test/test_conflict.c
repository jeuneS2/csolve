#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace conflict {
#include "../src/arith.c"
#include "../src/constr_types.c"
#include "../src/conflict.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD1(print_fatal, void (const char *));
  MOCK_METHOD0(bind_level_get, size_t (void));
  MOCK_METHOD2(clause_list_append, void (struct clause_list_t *,  struct wand_expr_t *));
#define CONSTR_TYPE_MOCKS(UPNAME, NAME, OP) \
  MOCK_METHOD1(eval_ ## NAME, const struct val_t(const struct constr_t *)); \
  MOCK_METHOD3(propagate_ ## NAME, prop_result_t(struct constr_t *, const struct val_t, const struct wand_expr_t *)); \
  MOCK_METHOD1(normal_ ## NAME, struct constr_t *(struct constr_t *));
  CONSTR_TYPE_LIST(CONSTR_TYPE_MOCKS)
};

Mock *MockProxy;

uint64_t calloc_max;
uint64_t confl;

size_t bind_level_get(void) {
  return MockProxy->bind_level_get();
}

void clause_list_append(struct clause_list_t *list, struct wand_expr_t *elem) {
  MockProxy->clause_list_append(list, elem);
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

#define CONSTR_TYPE_CMOCKS(UPNAME, NAME, OP)                            \
const struct val_t eval_ ## NAME(const struct constr_t *constr) {       \
  return MockProxy->eval_ ## NAME(constr);                              \
}                                                                       \
prop_result_t propagate_ ## NAME(struct constr_t *constr, struct val_t val, const struct wand_expr_t *clause) { \
  return MockProxy->propagate_ ## NAME(constr, val, clause);            \
}                                                                       \
struct constr_t *normal_ ## NAME(struct constr_t *constr) {             \
  return MockProxy->normal_ ## NAME(constr);                            \
}
CONSTR_TYPE_LIST(CONSTR_TYPE_CMOCKS)


TEST(ConflictAlloc, Init) {
  _alloc_stack = NULL;
  conflict_alloc_init(123);
  EXPECT_EQ(123U, _alloc_stack_size);
  EXPECT_NE((char *)NULL, _alloc_stack);
  free(_alloc_stack);

  _alloc_stack = NULL;
  conflict_alloc_init(17);
  EXPECT_EQ(17U, _alloc_stack_size);
  EXPECT_NE((char *)NULL, _alloc_stack);
  free(_alloc_stack);
}

TEST(ConflictAlloc, Free) {
  conflict_alloc_init(64);
  conflict_alloc_free();
  EXPECT_EQ(0U, _alloc_stack_size);
  EXPECT_EQ((char *)NULL, _alloc_stack);
}

TEST(ConflictAlloc, Success) {
  conflict_alloc_init(1024);
  calloc_max = 64;
  _alloc_stack_pointer = 32;
  size_t ptr = _alloc_stack_pointer;

  EXPECT_EQ(&_alloc_stack[ptr], conflict_alloc(NULL, 23));
  EXPECT_EQ(ptr + 23 + 1, _alloc_stack_pointer);
  EXPECT_EQ(64U, calloc_max);

  EXPECT_EQ(&_alloc_stack[ptr], conflict_alloc(&_alloc_stack[ptr], 27));
  EXPECT_EQ(ptr + 27 + 5, _alloc_stack_pointer);
  EXPECT_EQ(64U, calloc_max);

  EXPECT_EQ(&_alloc_stack[ptr + 27 + 5], conflict_alloc(NULL, 17));
  EXPECT_EQ(ptr + 27 + 5 + 17 + 7, _alloc_stack_pointer);
  EXPECT_EQ(ptr + 27 + 5 + 17 + 7, calloc_max);
}

TEST(ConflictAlloc, Fail) {
  MockProxy = new Mock();
  conflict_alloc_init(1024);
  _alloc_stack_pointer = 32;
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_OUT_OF_MEMORY)).Times(1);
  conflict_alloc(NULL, _alloc_stack_size);
  delete(MockProxy);
}

TEST(ConflictDealloc, Success) {
  conflict_alloc_init(1024);
  _alloc_stack_pointer = 64;
  conflict_dealloc(&_alloc_stack[24]);
  EXPECT_EQ(24U, _alloc_stack_pointer);
}

TEST(ConflictDealloc, Fail) {
  conflict_alloc_init(1024);
  _alloc_stack_pointer = 64;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  conflict_dealloc(&_alloc_stack[ALLOC_ALIGNMENT-1]);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  conflict_dealloc(&_alloc_stack[128]);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  conflict_dealloc(NULL);
  delete(MockProxy);
}

TEST(ConflictLevel, Basic) {
  _conflict_level = 77;
  EXPECT_EQ(77, conflict_level());
  _conflict_level = 33;
  EXPECT_EQ(33, conflict_level());
}

TEST(ConflictVar, Basic) {
  struct env_t v;
  struct env_t w;

  _conflict_var = &v;
  EXPECT_EQ(&v, conflict_var());
  _conflict_var = &w;
  EXPECT_EQ(&w, conflict_var());
}

TEST(ConflictReset, Basic) {
  struct env_t v;
  _conflict_level = 42;
  _conflict_var = &v;

  conflict_reset();
  EXPECT_EQ(SIZE_MAX, _conflict_level);
  EXPECT_EQ(NULL, _conflict_var);
}

TEST(ConflictSeen, Basic) {
  struct constr_t c1;
  struct constr_t c2;
  struct env_t v1;
  struct env_t v2;

  conflict_seen_reset();

  EXPECT_EQ(conflict_seen(&c1), false);
  EXPECT_EQ(conflict_seen(&c2), false);
  EXPECT_EQ(conflict_seen(&v1), false);
  EXPECT_EQ(conflict_seen(&v2), false);

  EXPECT_EQ(conflict_seen_add(&c1), CONFL_OK);

  EXPECT_EQ(conflict_seen(&c1), true);
  EXPECT_EQ(conflict_seen(&c2), false);
  EXPECT_EQ(conflict_seen(&v1), false);
  EXPECT_EQ(conflict_seen(&v2), false);

  EXPECT_EQ(conflict_seen_add(&c2), CONFL_OK);

  EXPECT_EQ(conflict_seen(&c1), true);
  EXPECT_EQ(conflict_seen(&c2), true);
  EXPECT_EQ(conflict_seen(&v1), false);
  EXPECT_EQ(conflict_seen(&v2), false);

  EXPECT_EQ(conflict_seen_add(&v1), CONFL_OK);

  EXPECT_EQ(conflict_seen(&c1), true);
  EXPECT_EQ(conflict_seen(&c2), true);
  EXPECT_EQ(conflict_seen(&v1), true);
  EXPECT_EQ(conflict_seen(&v2), false);

  EXPECT_EQ(conflict_seen_add(&v2), CONFL_OK);

  EXPECT_EQ(conflict_seen(&c1), true);
  EXPECT_EQ(conflict_seen(&c2), true);
  EXPECT_EQ(conflict_seen(&v1), true);
  EXPECT_EQ(conflict_seen(&v2), true);

  conflict_seen_reset();

  EXPECT_EQ(conflict_seen(&c1), false);
  EXPECT_EQ(conflict_seen(&c2), false);
  EXPECT_EQ(conflict_seen(&v1), false);
  EXPECT_EQ(conflict_seen(&v2), false);
}

TEST(ConflictAddTerm, Basic) {
  struct constr_t c1 = CONSTRAINT_TERM(VALUE(0));
  struct env_t e1 =  { .key = NULL, .val = &c1, .binds = NULL,
                       .clauses = { .length = 0, .elems = NULL },
                       .order = 0, .prio = 0, .level = 23 };
  c1.constr.term.env = &e1;

  struct constr_t c2 = CONSTRAINT_TERM(VALUE(1));
  struct env_t e2 =  { .key = NULL, .val = &c2, .binds = NULL,
                       .clauses = { .length = 0, .elems = NULL },
                       .order = 0, .prio = 0, .level = 17 };
  c2.constr.term.env = &e2;

  struct constr_t confl = CONSTRAINT_CONFL(0, NULL);

  _conflict_max_level = 0;
  EXPECT_EQ(CONFL_OK, conflict_add_term(&confl, &c1));
  EXPECT_EQ(1, confl.constr.confl.length);
  EXPECT_EQ(&c1, confl.constr.confl.elems[0].var);
  EXPECT_EQ(VALUE(0), confl.constr.confl.elems[0].val);
  EXPECT_EQ(_conflict_max_level, 23);

  EXPECT_EQ(CONFL_OK, conflict_add_term(&confl, &c2));
  EXPECT_EQ(2, confl.constr.confl.length);
  EXPECT_EQ(&c1, confl.constr.confl.elems[0].var);
  EXPECT_EQ(VALUE(0), confl.constr.confl.elems[0].val);
  EXPECT_EQ(&c2, confl.constr.confl.elems[1].var);
  EXPECT_EQ(VALUE(1), confl.constr.confl.elems[1].val);
  EXPECT_EQ(_conflict_max_level, 23);
}

TEST(ConflictAddTerm, Error) {
  struct constr_t c = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t d = CONSTRAINT_TERM(VALUE(2));
  struct constr_t e = CONSTRAINT_TERM(VALUE(-3));

  struct constr_t confl = CONSTRAINT_CONFL(0, NULL);

  EXPECT_EQ(CONFL_ERROR, conflict_add_term(&confl, &c));
  EXPECT_EQ(CONFL_ERROR, conflict_add_term(&confl, &d));
  EXPECT_EQ(CONFL_ERROR, conflict_add_term(&confl, &e));
}

 TEST(ConflictAddConstrTerm, Basic) {
  struct constr_t c1 = CONSTRAINT_TERM(VALUE(0));
  struct env_t var1 =  { .key = NULL, .val = &c1, .binds = NULL,
                         .clauses = { .length = 0, .elems = NULL },
                         .order = 0, .prio = 0, .level = 17 };
  c1.constr.term.env = NULL;

  struct constr_t c2 = CONSTRAINT_TERM(VALUE(1));
  struct env_t var2 =  { .key = NULL, .val = &c2, .binds = NULL,
                         .clauses = { .length = 0, .elems = NULL },
                         .order = 0, .prio = 0, .level = 23 };
  c2.constr.term.env = &var2;

  struct binding_t b3 = { .var = NULL, .val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX),
                          .level = SIZE_MAX, .clause = NULL, .prev = NULL };
  struct constr_t c3 = CONSTRAINT_TERM(VALUE(1));
  struct env_t var3 =  { .key = NULL, .val = &c3, .binds = &b3,
                         .clauses = { .length = 0, .elems = NULL },
                         .order = 0, .prio = 0, .level = 23 };
  c3.constr.term.env = &var3;

  struct constr_t constr = CONSTRAINT_TERM(INTERVAL(DOMAIN_MIN, DOMAIN_MAX));
  struct wand_expr_t cl = { &constr, &constr, 0 };
  struct binding_t b4 = { .var = NULL, .val = INTERVAL(DOMAIN_MIN, DOMAIN_MAX),
                          .level = SIZE_MAX, .clause = &cl, .prev = NULL };
  struct constr_t c4 = CONSTRAINT_TERM(VALUE(1));
  struct env_t var4 =  { .key = NULL, .val = &c4, .binds = &b4,
                         .clauses = { .length = 0, .elems = NULL },
                         .order = 0, .prio = 0, .level = 23 };
  c4.constr.term.env = &var4;

  struct constr_t confl = CONSTRAINT_CONFL(0, NULL);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var2, &confl, &c1));
  EXPECT_EQ(0, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), false);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), false);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var2, &confl, &c2));
  EXPECT_EQ(0, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), false);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), false);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_level_get()).Times(1).WillOnce(::testing::Return(0));
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var1, &confl, &c2));
  EXPECT_EQ(0, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), true);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), false);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_level_get()).Times(1).WillOnce(::testing::Return(66));
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var1, &confl, &c2));
  EXPECT_EQ(1, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), false);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), false);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_level_get()).Times(1).WillOnce(::testing::Return(0));
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var1, &confl, &c3));
  EXPECT_EQ(2, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), false);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), false);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, bind_level_get()).Times(1).WillOnce(::testing::Return(0));
  EXPECT_EQ(CONFL_OK, conflict_add_constr_term(&var1, &confl, &c4));
  EXPECT_EQ(2, confl.constr.confl.length);
  EXPECT_EQ(conflict_seen(&var1), false);
  EXPECT_EQ(conflict_seen(&var2), false);
  EXPECT_EQ(conflict_seen(&var3), false);
  EXPECT_EQ(conflict_seen(&var4), true);
  conflict_seen_reset();
  delete(MockProxy);
}

TEST(ConflictAddConstrWand, Basic) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct wand_expr_t E [2] = { { .constr = &A, .orig = &A, .prop_tag = 0 },
                               { .constr = &B, .orig = &B, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr_wand(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);
}

TEST(ConflictAddConstrConfl, Basic) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct confl_elem_t E [2] = { { .val = VALUE(0), .var = &A },
                                { .val = VALUE(0), .var = &B } };
  struct constr_t X = CONSTRAINT_CONFL(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr_confl(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);
}

TEST(ConflictAddConstrExpr, Basic) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(LT, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(ADD, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(MUL, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(AND, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(OR, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NEG, &A, NULL);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), false);
  conflict_seen_reset();
  delete(MockProxy);

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(NOT, &B, NULL);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), false);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);

  struct constr_type_t CONSTR_FOO = { NULL, NULL, NULL, (enum operator_t)-1 };
  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(FOO, &B, NULL);
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_INVALID_OPERATION)).Times(1);
  EXPECT_EQ(CONFL_OK, conflict_add_constr_expr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), false);
  EXPECT_EQ(conflict_seen(&B), false);
  conflict_seen_reset();
  delete(MockProxy);
}

TEST(ConflictAddConstr, Wand) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct wand_expr_t E [2] = { { .constr = &A, .orig = &A, .prop_tag = 0 },
                               { .constr = &B, .orig = &B, .prop_tag = 0 } };
  struct constr_t X = CONSTRAINT_WAND(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  EXPECT_EQ(CONFL_OK, conflict_add_constr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);
}

 TEST(ConflictAddConstr, Confl) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct confl_elem_t E [2] = { { .val = VALUE(0), .var = &A },
                                { .val = VALUE(0), .var = &B } };
  struct constr_t X = CONSTRAINT_CONFL(2, E);

  MockProxy = new Mock();
  EXPECT_EQ(CONFL_OK, conflict_add_constr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  EXPECT_EQ(CONFL_OK, conflict_add_constr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);
}

 TEST(ConflictAddConstr, Expr) {
  struct constr_t A = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t B = CONSTRAINT_TERM(INTERVAL(0, 1));
  struct constr_t X;

  MockProxy = new Mock();
  X = CONSTRAINT_EXPR(EQ, &A, &B);
  EXPECT_EQ(CONFL_OK, conflict_add_constr(NULL, NULL, &X));
  EXPECT_EQ(conflict_seen(&A), true);
  EXPECT_EQ(conflict_seen(&B), true);
  conflict_seen_reset();
  delete(MockProxy);
}

}
