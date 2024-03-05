#include "eventloop.h"

#include <unistd.h> // read()
#include <sys/eventfd.h>

#include "channel.h"

using namespace tiny_muduo;

EventLoop::EventLoop()
    : tid_(::pthread_self()),
      epoller_(new Epoller()),
      wakeup_fd_(::eventfd(0, EFD_NONBLOCK)),
      wakeup_channel_(new Channel(this, wakeup_fd_)) {
    printf("[Cstr]: EventLoop\n");
    wakeup_channel_->SetReadCallback(std::bind(&EventLoop::HandleRead, this));
    wakeup_channel_->EnableReading();
}

EventLoop::~EventLoop() {}


// 无限循环，不断地调用epoller_的Poll方法进行事件轮询
// 在事件就绪后调用相应通道的HandleEvent方法处理事件。
// 然后清空活跃通道列表，准备下一轮事件轮询。
void EventLoop::Loop() {
    while(1) {
        epoller_->Poll(active_channels_);
        printf("EventLoop::Loop eventnum %ld\n", active_channels_.size());
        for(const auto& channel: active_channels_) {
            printf("EventLoop::Loop connfd %d\n", channel->fd());
            channel->HandleEvent();
        }
        active_channels_.clear();
        DoToDoList();
    }
}

void EventLoop::HandleRead() {
    uint64_t read_one_byte = 1;
    ::read(wakeup_fd_, &read_one_byte, sizeof(read_one_byte));
    return;
}

void EventLoop::RunOneFunc(const BasicFunc& func) {
    if(IsInThreadPool()) {
        func();
    } else {
        {
            MutexLockGuard lock(mutex_);
            to_do_list_.emplace_back(func);
        }
        if(!IsInThreadPool()) {
            uint64_t write_one_byte = 1;
            ::write(wakeup_fd_, &write_one_byte, sizeof(write_one_byte));
        }
    }
}

void EventLoop::DoToDoList() {
    ToDoList functors;
    {
        MutexLockGuard lock(mutex_);
        functors.swap(to_do_list_);
    }

    for(const auto& func: functors) {
        func();
    }
}

