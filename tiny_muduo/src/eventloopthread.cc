#include "eventloopthread.h"

#include <pthread.h>

#include <functional>

#include "condition.h"
#include "eventloop.h"
#include "mutex.h"

using namespace tiny_muduo;

EventLoopThread::EventLoopThread()
    : loop_(nullptr),
      thread_(std::bind(&EventLoopThread::StartFunc, this)),
      mutex_(),
      cond_(mutex_) {
  LOG_INFO("[Cstr]: EventLoopThread");
}

EventLoopThread::~EventLoopThread() {}

EventLoop* EventLoopThread::StartLoop() {
  thread_.StartThread();
  EventLoop* loop = nullptr;
  {
    MutexLockGuard lock(mutex_);
    while (loop_ == nullptr) {
      cond_.Wait();
    }
    loop = loop_;
  }
  return loop;
}

void EventLoopThread::StartFunc() {
  EventLoop loop;

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.Signal();
  }

  loop_->Loop();

  {
    MutexLockGuard lock(mutex_);
    loop_ = nullptr;
  }
}