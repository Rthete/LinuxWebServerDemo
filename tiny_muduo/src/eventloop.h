#ifndef TINY_MUDUO_EVENTLOOP_H_
#define TINY_MUDUO_EVENTLOOP_H_

#include <vector>
#include <functional>

#include "epoller.h"

namespace tiny_muduo
{
class Epoller;
class Channel;

class EventLoop {
public:
    typedef std::vector<Channel*> Channels;
    typedef std::function<void()> BasicFunc;

    EventLoop();
    ~EventLoop();

    void Loop();
    // 更新channel中的事件状态
    void Update(Channel* channel) { epoller_->Update(channel); }
    // 在事件循环中执行一个指定的函数，允许在事件处理过程中执行额外的用户指定的操作，通过函数对象作为参数传入。
    void RunOneFunc(const BasicFunc& func) { func(); }

private:
    Epoller* epoller_;
    int wakeup_fd_;
    Channels active_channels_;
};
} // namespace tiny_muduo

#endif