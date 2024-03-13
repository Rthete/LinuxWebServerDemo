#ifndef TINY_MODUO_EVENTLOOPTHREADPOOL_H_
#define TINY_MODUO_EVENTLOOPTHREADPOOL_H_

#include <vector>

#include "noncopyable.h"

namespace tiny_muduo {
class EventLoopThread;
class EventLoop;

class EventLoopThreadPool : public NoncopyAble {
 public:
  typedef std::vector<EventLoopThread*> Thread;
  typedef std::vector<EventLoop*> Loop;

  EventLoopThreadPool(EventLoop* loop);
  ~EventLoopThreadPool();

  void SetThreadNums(int thread_nums) { thread_nums_ = thread_nums; }

  void StartLoop();
  EventLoop* NextLoop();

 private:
  EventLoop* base_loop_;
  Thread threads_;
  Loop loops_;

  int thread_nums_;
  int next_;
};
}  // namespace tiny_muduo

#endif