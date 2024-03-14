#ifndef TINY_MUDUO_THREAD_H
#define TINY_MUDUO_THREAD_H

#include <pthread.h>

#include <functional>

#include "latch.h"
#include "log.h"
#include "noncopyable.h"

namespace tiny_muduo {
class Thread : public NoncopyAble {
 public:
  typedef std::function<void()> ThreadFunc;

  Thread(const ThreadFunc& func);
  ~Thread();

  void StartThread();

 private:
  pthread_t pthread_id_;
  ThreadFunc func_;
  Latch latch_;
};

struct ThreadData {
  typedef tiny_muduo::Thread::ThreadFunc ThreadFunc;

  ThreadFunc func_;
  Latch* latch_;

  ThreadData(ThreadFunc& func, Latch* latch) : func_(func), latch_(latch) {}

  void RunOneFunc() {
    latch_->CountDown();
    latch_ = nullptr;
    func_();
  }
};

}  // namespace tiny_muduo

#endif