#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace patch {
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

TEST(Patch, Init) {
  _patch_stack = NULL;
  patch_init(123);
  EXPECT_EQ(123U, _patch_stack_size);
  EXPECT_NE((struct patching_t *)NULL, _patch_stack);
  free(_patch_stack);

  _patch_stack = NULL;
  patch_init(17);
  EXPECT_EQ(17U, _patch_stack_size);
  EXPECT_NE((struct patching_t *)NULL, _patch_stack);
  free(_patch_stack);
}

TEST(Patch, Free) {
  patch_init(64);
  patch_free();
  EXPECT_EQ(0U, _patch_stack_size);
  EXPECT_EQ((struct patching_t *)NULL, _patch_stack);
}

TEST(Patch, Success) {
  struct constr_t c;
  struct constr_t d;
  struct wand_expr_t loc = { .constr = &c, .orig = &c, .prop_tag = 0 };

  patch_init(64);

  MockProxy = new Mock();
  _patch_depth = 23;
  patch(&loc, &d);
  EXPECT_EQ(24U, _patch_depth);
  EXPECT_EQ(loc.constr, &d);
  EXPECT_EQ(loc.orig, &c);
  delete(MockProxy);
}

TEST(Patch, Fail) {
  struct constr_t c;
  struct wand_expr_t loc;

  patch_init(64);

  MockProxy = new Mock();
  _patch_depth = _patch_stack_size;
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_TOO_MANY_PATCHES)).Times(1);
  patch(&loc, &c);
  delete(MockProxy);
}

TEST(Unpatch, Sucess) {
  struct constr_t c1;
  struct constr_t d1;
  struct wand_expr_t loc1 = { .constr = &c1, .orig = &c1, .prop_tag = 0 };

  struct constr_t c2;
  struct constr_t d2;
  struct wand_expr_t loc2 = { .constr = &c2, .orig = &c2, .prop_tag = 0 };

  patch_init(64);

  _patch_depth = 17;
  MockProxy = new Mock();
  patch(&loc1, &d1);
  patch(&loc2, &d2);
  delete(MockProxy);

  MockProxy = new Mock();
  unpatch(17);
  EXPECT_EQ(loc1.constr, &c1);
  EXPECT_EQ(loc2.constr, &c2);
  EXPECT_EQ(17U, _patch_depth);
  delete(MockProxy);
}

} // end namespace
