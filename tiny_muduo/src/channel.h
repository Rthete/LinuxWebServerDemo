#ifndef TINY_MUDUO_CHANNEL_H_
#define TINY_MUDUO_CHANNEL_H_

#include <sys/epoll.h>

#include <utility>

#include "callback.h"
#include "eventloop.h"

namespace tiny_muduo {
enum ChannelState { kNew, kAdded, kDeleted };

class Channel {
 public:
  Channel(EventLoop* loop, const int& fd);
  ~Channel();

  // 根据事件类型调用相应回调函数(读or写)
  void HandleEvent();
  // 设置读事件回调函数
  void SetReadCallback(const ReadCallback& callback) {
    read_callback_ = std::move(callback);
  }
  // 设置写事件回调函数(在这个库中并没有使用)
  void SetWriteCallback(const WriteCallback& callback) {
    write_callback_ = std::move(callback);
  }
  // 启用读事件的监听
  void EnableReading() {
    events_ |= (EPOLLIN | EPOLLPRI);
    Update();
  }
  // 启用写事件的监听
  void EnableWriting() {
    events_ |= EPOLLOUT;
    Update();
  }

  void DisableWriting() {
    events_ &= ~EPOLLOUT;
    Update();
  }

  void DisableAll() {
    events_ = 0;
    Update();
  }

  void RemoveFd() {}
  // 更新当前channel的事件监听状态
  void Update() { loop_->Update(this); }
  // 设置从epoll收到的事件类型
  void SetReceivedEvents(int events) { recv_events_ = events; }

  void SetChannelState(ChannelState state) { state_ = state; }

  int fd() { return fd_; }
  int events() { return events_; }
  int recv_events() { return recv_events_; }
  ChannelState state() { return state_; }

  bool IsWriting() { return events_ & EPOLLOUT; }
  bool IsReading() { return events_ & (EPOLLIN | EPOLLPRI); }

 private:
  EventLoop* loop_;
  int fd_;
  int events_;       // update events
  int recv_events_;  // epoll received events

  ChannelState state_;
  ReadCallback read_callback_;
  WriteCallback write_callback_;
};

}  // namespace tiny_muduo

#endif