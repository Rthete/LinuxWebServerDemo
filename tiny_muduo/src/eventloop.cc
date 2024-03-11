#include "eventloop.h"

#include <sys/eventfd.h>
#include <unistd.h>  // read()

#include "channel.h"

using namespace tiny_muduo;

EventLoop::EventLoop()
    : tid_(::pthread_self()),
      epoller_(new Epoller()),
      wakeup_fd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeup_channel_(new Channel(this, wakeup_fd_)),
      calling_functors_(false) {
  printf("[Cstr]: EventLoop\n");
  wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
  wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {
  wakeup_channel_->DisableAll();
  Remove(wakeup_channel_.get());
}

// 无限循环，不断地调用epoller_的Poll方法进行事件轮询
// 在事件就绪后调用相应通道的HandleEvent方法处理事件。
// 然后清空活跃通道列表，准备下一轮事件轮询。
void EventLoop::Loop() {
  while (1) {
    active_channels_.clear();
    epoller_->Poll(active_channels_);
    printf("EventLoop::Loop eventnum %ld\n", active_channels_.size());
    for (const auto& channel : active_channels_) {
      printf("EventLoop::Loop connfd %d\n", channel->fd());
      channel->HandleEvent();
    }
    DoToDoList();
  }
}

void EventLoop::HandleRead() {
  uint64_t read_one_byte = 1;
  ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
  return;
}

void EventLoop::QueueOneFunc(BasicFunc func) {
  {
    MutexLockGuard lock(mutex_);
    to_do_list_.emplace_back(std::move(func));
  }

  if (!IsInThreadPool() || calling_functors_) {
    uint64_t write_one_byte = 1;
    ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
  }
}

void EventLoop::RunOneFunc(BasicFunc func) {
  if (IsInThreadPool()) {
    func();
  } else {
    QueueOneFunc(std::move(func));
  }
}

void EventLoop::DoToDoList() {
  ToDoList functors;
  calling_functors_ = true;
  {
    MutexLockGuard lock(mutex_);
    functors.swap(to_do_list_);
  }

  for (const auto& func : functors) {
    func();
  }
  calling_functors_ = false;
}
