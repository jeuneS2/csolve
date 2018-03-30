#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace alloc {
#include "../src/util.c"

class Mock {
 public:
  MOCK_METHOD0(eval_cache_invalidate, void(void));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

TEST(Alloc, Success) {
  alloc_max = 64;
  alloc_stack_pointer = 32;
  size_t ptr = alloc_stack_pointer;

  EXPECT_EQ(&alloc_stack[ptr], alloc(23));
  ptr += 23+1;
  EXPECT_EQ(ptr, alloc_stack_pointer);
  EXPECT_EQ(64, alloc_max);

  EXPECT_EQ(&alloc_stack[ptr], alloc(17));
  ptr += 17+7;
  EXPECT_EQ(ptr, alloc_stack_pointer);
  EXPECT_EQ(ptr, alloc_max);
}

TEST(Alloc, Fail) {
  std::string output;

  alloc_stack_pointer = 32;
  testing::internal::CaptureStderr();
  EXPECT_EQ(NULL, alloc(ALLOC_STACK_SIZE));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: out of memory\n");
}

TEST(Dealloc, Success) {
  alloc_stack_pointer = 64;
  dealloc(&alloc_stack[24]);
  EXPECT_EQ(24, alloc_stack_pointer);
}

TEST(Dealloc, Fail) {
  std::string output;

  alloc_stack_pointer = 64;

  testing::internal::CaptureStderr();
  dealloc(&alloc_stack[ALLOC_ALIGNMENT-1]);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");

  testing::internal::CaptureStderr();
  dealloc(&alloc_stack[128]);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");

  testing::internal::CaptureStderr();
  dealloc(NULL);
  EXPECT_EQ(64, alloc_stack_pointer);
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: wrong deallocation\n");
}

} // end namespace
