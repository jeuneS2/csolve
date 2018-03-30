#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace bind {
#include "../src/util.c"

bool operator==(const struct val_t& lhs, const struct val_t& rhs) {
  return lhs.type == rhs.type && memcmp(&lhs.value, &rhs.value, sizeof(lhs.value)) == 0;
}

class Mock {
 public:
  MOCK_METHOD0(eval_cache_invalidate, void(void));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

TEST(Bind, Success) {
  struct val_t loc = INTERVAL(0, 100);

  bind_depth = 23;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(1);
  EXPECT_EQ(23, bind(&loc, VALUE(17)));
  EXPECT_EQ(24, bind_depth);
  EXPECT_EQ(loc, VALUE(17));
  delete(MockProxy);
}

TEST(Bind, Fail) {
  std::string output;
  struct val_t loc = INTERVAL(0, 100);

  bind_depth = 23;
  testing::internal::CaptureStderr();
  EXPECT_EQ(MAX_BINDS, bind(NULL, VALUE(17)));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: cannot bind NULL\n");

  bind_depth = MAX_BINDS;
  testing::internal::CaptureStderr();
  EXPECT_EQ(MAX_BINDS, bind(&loc, VALUE(17)));
  output = testing::internal::GetCapturedStderr();
  EXPECT_EQ(output, "ERROR: exceeded maximum number of binds\n");
}

TEST(Unbind, Sucess) {
  struct val_t loc1 = INTERVAL(0, 100);
  struct val_t loc2 = INTERVAL(17, 23);

  bind_depth = 17;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(2);
  bind(&loc1, VALUE(42));
  bind(&loc2, VALUE(23));
  delete(MockProxy);

  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(2);
  unbind(17);
  EXPECT_EQ(loc1, INTERVAL(0, 100));
  EXPECT_EQ(loc2, INTERVAL(17, 23));
  EXPECT_EQ(17, bind_depth);
  delete(MockProxy);
}

} // end namespace
