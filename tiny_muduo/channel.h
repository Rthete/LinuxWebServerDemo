#ifndef TINY_MUDUO_CHANNEL_H_
#define TINY_MUDUO_CHANNEL_H_

#include <sys/epoll.h>

#include "eventloop.h"
#include "callback.h"

namespace tiny_muduo
{
class Channel {
public:
    Channel(EventLoop* loop, const int& fd);
    ~Channel();

    // 根据事件类型调用相应回调函数(读or写)
    void HandleEvent();
    // 设置读事件回调函数
    void SetReadCallback(const ReadCallback& callback) {
        read_callback_ = callback;
    }
    // 设置写事件回调函数(在这个库中并没有使用)
    void SetWriteCallback(const WriteCallback& callback) {
        write_callback_ = callback;
    }
    // 启用读事件的监听
    void EnableReading() {
        events_ |= EPOLLIN;
        Update();
    }
    // 启用写事件的监听
    void EnableWriting() {
        events_ |= EPOLLOUT;
        Update();
    }
    // 更新当前channel的事件监听状态
    void Update() {
        loop_->Update(this);
    }
    // 设置从epoll收到的事件类型
    void SetReceivedEvents(int events) {
        recv_events_ = events;
    }
    
    int fd() { return fd_; }
    int events() { return events_; }
    int recv_events() { return recv_events_; }

private:
    EventLoop* loop_;
    int fd_;
    int events_;        // update events
    int recv_events_;   // epoll received events

    ReadCallback read_callback_;
    WriteCallback write_callback_;
};

} // namespace tiny_muduo


#endif