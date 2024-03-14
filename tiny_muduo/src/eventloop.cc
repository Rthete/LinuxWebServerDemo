#include "eventloop.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>  // read()

#include "channel.h"

using namespace tiny_muduo;

namespace {
/** SIGPIPE
 * 当程序尝试向一个已经关闭的管道写入数据时，系统会产生 SIGPIPE 信号，
 * 如果不处理这个信号，程序默认会终止。
 * 通过忽略 SIGPIPE 信号，程序可以继续运行而不受到管道关闭的影响。
 */
class IgnoreSigPipe {
 public:
  IgnoreSigPipe() { ::signal(SIGPIPE, SIG_IGN); }
};

IgnoreSigPipe initObj;
}  // namespace

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
  close(wakeup_fd_);
}

// 无限循环，不断地调用epoller_的Poll方法进行事件轮询
// 在事件就绪后调用相应通道的HandleEvent方法处理事件。
// 然后清空活跃通道列表，准备下一轮事件轮询。
void EventLoop::Loop() {
  assert(IsInThreadPool());
  while (true) {
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
  ssize_t read_size = ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
  (void)read_size;
  assert(read_size == sizeof(read_one_byte));
  return;
}

void EventLoop::QueueOneFunc(BasicFunc func) {
  {
    MutexLockGuard lock(mutex_);
    to_do_list_.emplace_back(std::move(func));
  }

  if (!IsInThreadPool() || calling_functors_) {
    uint64_t write_one_byte = 1;
    int write_size =
        ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
    (void)write_size;
    assert(write_size == sizeof(write_one_byte));
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
