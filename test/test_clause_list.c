#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace clause_list {
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

TEST(ClauseListContains, Found) {
  struct wand_expr_t w1;
  struct wand_expr_t w2;

  struct wand_expr_t *e [2] = { &w1, &w2 };
  struct clause_list_t list = { .length = 2, .elems = e };

  EXPECT_EQ(true, clause_list_contains(&list, &w1));
  EXPECT_EQ(true, clause_list_contains(&list, &w2));
}

TEST(ClauseListContains, NotFound) {
  struct wand_expr_t w1;
  struct wand_expr_t w2;
  struct wand_expr_t w3;
  struct wand_expr_t w4;

  struct wand_expr_t *e [2] = { &w1, &w2 };
  struct clause_list_t list = { .length = 2, .elems = e };

  EXPECT_EQ(false, clause_list_contains(&list, &w3));
  EXPECT_EQ(false, clause_list_contains(&list, &w4));
}

TEST(ClauseList, Append) {
  struct wand_expr_t w1;
  struct wand_expr_t w2;

  struct clause_list_t list = { .length = 0, .elems = NULL };

  clause_list_append(&list, &w1);
  EXPECT_EQ(1, list.length);
  EXPECT_EQ(&w1, list.elems[0]);

  clause_list_append(&list, &w2);
  EXPECT_EQ(2, list.length);
  EXPECT_EQ(&w1, list.elems[0]);
  EXPECT_EQ(&w2, list.elems[1]);

  clause_list_append(&list, &w1);
  EXPECT_EQ(3, list.length);
  EXPECT_EQ(&w1, list.elems[0]);
  EXPECT_EQ(&w2, list.elems[1]);
}

} // end namespace
