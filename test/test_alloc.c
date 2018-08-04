#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace alloc {
#include "../src/util.c"

class Mock {
 public:
  MOCK_METHOD0(eval_cache_invalidate, void(void));
  MOCK_METHOD1(print_fatal, void (const char *));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

TEST(Alloc, Init) {
  _alloc_stack = NULL;
  alloc_init(123);
  EXPECT_EQ(123, _alloc_stack_size);
  EXPECT_NE((char *)NULL, _alloc_stack);
  free(_alloc_stack);

  _alloc_stack = NULL;
  alloc_init(17);
  EXPECT_EQ(17, _alloc_stack_size);
  EXPECT_NE((char *)NULL, _alloc_stack);
  free(_alloc_stack);
}

TEST(Alloc, Free) {
  alloc_init(64);
  alloc_free();
  EXPECT_EQ(0, _alloc_stack_size);
  EXPECT_EQ((char *)NULL, _alloc_stack);
}

TEST(Alloc, Success) {
  alloc_init(1024);
  alloc_max = 64;
  _alloc_stack_pointer = 32;
  size_t ptr = _alloc_stack_pointer;

  EXPECT_EQ(&_alloc_stack[ptr], alloc(23));
  ptr += 23+1;
  EXPECT_EQ(ptr, _alloc_stack_pointer);
  EXPECT_EQ(64, alloc_max);

  EXPECT_EQ(&_alloc_stack[ptr], alloc(17));
  ptr += 17+7;
  EXPECT_EQ(ptr, _alloc_stack_pointer);
  EXPECT_EQ(ptr, alloc_max);
}

TEST(Alloc, Fail) {
  MockProxy = new Mock();
  alloc_init(1024);
  _alloc_stack_pointer = 32;
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_OUT_OF_MEMORY)).Times(1);
  alloc(_alloc_stack_size);
  delete(MockProxy);
}

TEST(Dealloc, Success) {
  alloc_init(1024);
  _alloc_stack_pointer = 64;
  dealloc(&_alloc_stack[24]);
  EXPECT_EQ(24, _alloc_stack_pointer);
}

TEST(Dealloc, Fail) {
  alloc_init(1024);
  _alloc_stack_pointer = 64;

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  dealloc(&_alloc_stack[ALLOC_ALIGNMENT-1]);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  dealloc(&_alloc_stack[128]);
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, print_fatal(ERROR_MSG_WRONG_DEALLOC)).Times(1);
  dealloc(NULL);
  delete(MockProxy);
}

} // end namespace
