#ifndef TINY_MODUO_EVENTLOOPTHREAD_H_
#define TINY_MODUO_EVENTLOOPTHREAD_H_

#include "condition.h"
#include "mutex.h"
#include "thread.h"

namespace tiny_muduo {
class EventLoop;
class EventLoopThread {
 public:
  EventLoopThread();
  ~EventLoopThread();

  void StartFunc();
  EventLoop* StartLoop();

 private:
  EventLoop* loop_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
};
}  // namespace tiny_muduo

#endif