#include "eventloopthreadpool.h"

#include "eventloopthread.h"

using namespace tiny_muduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* loop)
    : base_loop_(loop),
      threads_(),
      loops_(),
      thread_nums_(0),
      next_(0) {
    printf("[Cstr]: EventLoopThreadPool\n");
}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::StartLoop() {
    // 每个子线程做一个Epoller和EventLoop，等待连接请求
    for(int i = 0; i < thread_nums_; i++) {
        EventLoopThread* ptr = new EventLoopThread();
        threads_.emplace_back(ptr);
        loops_.emplace_back(ptr->StartLoop());
    }
}

// round robin轮流选择线程，返回对应的loop对象
EventLoop* EventLoopThreadPool::NextLoop() {
    EventLoop* ret = base_loop_;
    if(!loops_.empty()) {
        ret = loops_[next_];
        next_ = (next_ + 1) % thread_nums_;
    }
    return ret;
}
