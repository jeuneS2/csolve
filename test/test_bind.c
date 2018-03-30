#include <stdarg.h>
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
  MOCK_METHOD2(print_error, void (const char *, va_list));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

void print_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  MockProxy->print_error(fmt, args);
  va_end(args);
}

TEST(Bind, Success) {
  struct val_t loc = INTERVAL(0, 100);

  MockProxy = new Mock();
  bind_depth = 23;
  EXPECT_CALL(*MockProxy, eval_cache_invalidate())
    .Times(1);
  EXPECT_EQ(23, bind(&loc, VALUE(17)));
  EXPECT_EQ(24, bind_depth);
  EXPECT_EQ(loc, VALUE(17));
  delete(MockProxy);
}

TEST(Bind, Fail) {
  struct val_t loc = INTERVAL(0, 100);

  MockProxy = new Mock();
  bind_depth = 23;
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_NULL_BIND, testing::_)).Times(1);
  EXPECT_EQ(MAX_BINDS, bind(NULL, VALUE(17)));
  delete(MockProxy);

  MockProxy = new Mock();
  bind_depth = MAX_BINDS;
  EXPECT_CALL(*MockProxy, print_error(ERROR_MSG_TOO_MANY_BINDS, testing::_)).Times(1);
  EXPECT_EQ(MAX_BINDS, bind(&loc, VALUE(17)));
  delete(MockProxy);
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
