#include "eventloop.h"

#include "channel.h"

namespace tiny_muduo
{
EventLoop::EventLoop()
    : epoller_(new Epoller()),
      wakeup_fd_(0) {
}

EventLoop::~EventLoop() {}


// 无限循环，不断地调用epoller_的Poll方法进行事件轮询
// 在事件就绪后调用相应通道的HandleEvent方法处理事件。
// 然后清空活跃通道列表，准备下一轮事件轮询。
void EventLoop::Loop() {
    while(1) {
        epoller_->Poll(active_channels_);
        printf("EventLoop Loop:: eventnum %d\n", active_channels_.size());
        for(const auto& channel: active_channels_) {
            printf("EventLoop Loop connfd %d\n", channel->fd());
            channel->HandleEvent();
        }
        active_channels_.clear();
    }
}

} // namespace tiny_muduo
