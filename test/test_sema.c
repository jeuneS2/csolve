#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace sema {
#include <semaphore.h>
int mock_sem_init(sem_t *sem, int pshared, unsigned int value);
int mock_sem_wait(sem_t *sem);
int mock_sem_post(sem_t *sem);
#define sem_init mock_sem_init
#define sem_wait mock_sem_wait
#define sem_post mock_sem_post

#include "../src/util.c"

class Mock {
 public:
  MOCK_METHOD0(eval_cache_invalidate, void(void));
  MOCK_METHOD1(print_fatal, void (const char *));
  MOCK_METHOD3(sem_init, int(sem_t *, int, unsigned int));
  MOCK_METHOD1(sem_wait, int(sem_t *));
  MOCK_METHOD1(sem_post, int(sem_t *));
};

Mock *MockProxy;

void eval_cache_invalidate(void) {
  MockProxy->eval_cache_invalidate();
}

void print_fatal(const char *fmt, ...) {
  MockProxy->print_fatal(fmt);
}

int sem_init(sem_t *sem, int pshared, unsigned int value) {
  return MockProxy->sem_init(sem, pshared, value);
}

int sem_wait(sem_t *sem) {
  return MockProxy->sem_wait(sem);
}

int sem_post(sem_t *sem) {
  return MockProxy->sem_post(sem);
}

TEST(SemaInit, Basic) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_init(&sema, 1, 1)).Times(1).WillOnce(::testing::Return(0));
  sema_init(&sema);
  delete(MockProxy);
}

TEST(SemaInit, Error) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_init(&sema, 1, 1)).Times(1).WillOnce(::testing::Return(-1));
  EXPECT_CALL(*MockProxy, print_fatal("%s"));
  sema_init(&sema);
  delete(MockProxy);
}

TEST(SemaWait, Basic) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_wait(&sema)).Times(1).WillOnce(::testing::Return(0));
  sema_wait(&sema);
  delete(MockProxy);
}

TEST(SemaWait, Error) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_wait(&sema)).Times(1).WillOnce(::testing::Return(-1));
  EXPECT_CALL(*MockProxy, print_fatal("%s"));
  sema_wait(&sema);
  delete(MockProxy);
}

TEST(SemaPost, Basic) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_post(&sema)).Times(1).WillOnce(::testing::Return(0));
  sema_post(&sema);
  delete(MockProxy);
}

TEST(SemaPost, Error) {
  sem_t sema;
  MockProxy = new Mock();
  EXPECT_CALL(*MockProxy, sem_post(&sema)).Times(1).WillOnce(::testing::Return(-1));
  EXPECT_CALL(*MockProxy, print_fatal("%s"));
  sema_post(&sema);
  delete(MockProxy);
}

}
