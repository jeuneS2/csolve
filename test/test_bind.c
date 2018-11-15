#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace bind {
#include "../src/util.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
}

class Mock {
 public:
  MOCK_METHOD0(eval_cache_invalidate, void(void));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

size_t alloc_max;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

TEST(Bind, Init) {
  _bind_stack = NULL;
  bind_init(123);
  EXPECT_EQ(123U, _bind_stack_size);
  EXPECT_NE((struct binding_t *)NULL, _bind_stack);
  free(_bind_stack);

  _bind_stack = NULL;
  bind_init(17);
  EXPECT_EQ(17U, _bind_stack_size);
  EXPECT_NE((struct binding_t *)NULL, _bind_stack);
  free(_bind_stack);
}

TEST(Bind, Free) {
  bind_init(64);
  bind_free();
  EXPECT_EQ(0U, _bind_stack_size);
  EXPECT_EQ((struct binding_t *)NULL, _bind_stack);
}

TEST(Bind, Success) {
  struct constr_t c;
  struct env_t loc = { .key = "x", .val = &c, .binds = NULL,
                       .clauses = { .length = 0, .elems = NULL },
                       .order = 0, .prio = 0, .level = 0 };

  bind_init(64);

  MockProxy = new Mock();
  _bind_depth = 23;
  bind(&loc, VALUE(17), NULL);
  EXPECT_EQ(24U, _bind_depth);
  EXPECT_EQ(loc.val->constr.term.val, VALUE(17));
  delete(MockProxy);
}

TEST(Bind, Fail) {
  struct env_t loc;

  bind_init(64);

  MockProxy = new Mock();
  _bind_depth = 23;
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_NULL_BIND)).Times(1);
  bind(NULL, VALUE(17), NULL);
  delete(MockProxy);

  MockProxy = new Mock();
  _bind_depth = _bind_stack_size;
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_TOO_MANY_BINDS)).Times(1);
  bind(&loc, VALUE(17), NULL);
  delete(MockProxy);
}

TEST(Unbind, Sucess) {
  struct constr_t c1;
  struct env_t loc1 = { .key = "x1", .val = &c1, .binds = NULL,
                        .clauses = { .length = 0, .elems = NULL },
                        .order = 0, .prio = 0, .level = 0 };
  struct constr_t c2;
  struct env_t loc2 = { .key = "x2", .val = &c2, .binds = NULL,
                        .clauses = { .length = 0, .elems = NULL },
                        .order = 0, .prio = 0, .level = 0 };

  bind_init(64);

  _bind_depth = 17;
  MockProxy = new Mock();
  bind(&loc1, VALUE(42), NULL);
  bind(&loc2, VALUE(23), NULL);
  delete(MockProxy);

  MockProxy = new Mock();
  unbind(17);
  EXPECT_EQ(loc1.val, &c1);
  EXPECT_EQ(loc2.val, &c2);
  EXPECT_EQ(17U, _bind_depth);
  delete(MockProxy);
}

} // end namespace
