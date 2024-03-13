#ifndef TINY_MUDUO_LATCH_H_
#define TINY_MUDUO_LATCH_H_

#include "condition.h"
#include "mutex.h"
#include "noncopyable.h"

namespace tiny_muduo {

class Latch : public NoncopyAble {
 public:
  explicit Latch(int count) : count_(count), mutex_(), cond_(mutex_) {}

  void CountDown() {
    MutexLockGuard lock(mutex_);
    --count_;
    if (count_ == 0) {
      cond_.BroadCast();
    }
  }

  void Wait() {
    MutexLockGuard lock(mutex_);
    while (count_ > 0) {
      cond_.Wait();
    }
  }

 private:
  int count_;
  MutexLock mutex_;
  Condition cond_;
};

}  // namespace tiny_muduo
#endif
