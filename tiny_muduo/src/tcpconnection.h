#ifndef TINY_MODUO_TCPCONNECTION_H_
#define TINY_MODUO_TCPCONNECTION_H_

#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <utility>

#include "buffer.h"
#include "callback.h"
#include "channel.h"
#include "httpcontent.h"
#include "noncopyable.h"

using std::string;

namespace tiny_muduo {
class EventLoop;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      NoncopyAble {
 public:
  enum ConnectionState { kConnected, kDisconnected };

  TcpConnection(EventLoop* loop, int connfd);
  ~TcpConnection();

  // 设置连接建立时的回调函数
  void SetConnectionCallback(ConnectionCallback&& callback) {
    connection_callback_ = std::move(callback);
  }
  void SetConnectionCallback(const ConnectionCallback& callback) {
    connection_callback_ = callback;
  }
  // 设置消息到来时的回调函数
  void SetMessageCallback(MessageCallback&& callback) {
    message_callback_ = std::move(callback);
  }
  void SetMessageCallback(const MessageCallback& callback) {
    message_callback_ = callback;
  }
  // 设置连接关闭时的回调函数
  void SetCloseCallback(CloseCallback&& callback) {
    close_callback_ = std::move(callback);
  }
  void SetCloseCallback(const CloseCallback& callback) {
    close_callback_ = callback;
  }
  // 表示连接建立，启用读事件监听，并调用连接建立的回调函数通知上层代码连接已建立。
  void ConnectionEstablished() {
    state_ = kConnected;
    channel_->EnableReading();
    connection_callback_(shared_from_this(), &input_buffer_);
  }

  HttpContent* GetHttpContent() { return &content_; }

  void Shutdown();
  bool IsShutdown() const { return shutdown_state_; }
  void ConnectionDestructor();
  void HandleClose();
  void HandleMessage();
  void HandleWrite();
  void Send(Buffer* buffer);
  void Send(const string& str);
  void Send(const char* message, int len);
  void Send(const char* message) { Send(message, strlen(message)); }

  int fd() const { return fd_; }
  EventLoop* loop() const { return loop_; }

 private:
  EventLoop* loop_;
  int fd_;
  ConnectionState state_;
  std::unique_ptr<Channel> channel_;
  Buffer input_buffer_;
  Buffer output_buffer_;
  HttpContent content_;
  bool shutdown_state_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  CloseCallback close_callback_;
};

typedef std::shared_ptr<TcpConnection> TcpconnectionPtr;

}  // namespace tiny_muduo

#endif