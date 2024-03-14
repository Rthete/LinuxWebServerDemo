#ifndef TINY_MODUO_ECHO_H_
#define TINY_MODUO_ECHO_H_

#include <stdio.h>

#include <string>

#include "tcpconnection.h"
#include "tcpserver.h"

static const int thread_nums = 8;

namespace tiny_muduo {
class Address;
class EventLoop;

}  // namespace tiny_muduo

class EchoServer {
 public:
  EchoServer(tiny_muduo::EventLoop* loop,
             const tiny_muduo::Address& listen_addr);
  ~EchoServer() {}

  // 启动回显服务器，实际上是调用了TCPServer的Start方法开始监听连接。
  void Start() {
    // printf("Start in Echo\n");
    server_.Start();
  }

  // 连接建立时的回调函数。在这里，它简单地打印一条连接建立的信息。
  void ConnectionCallback(tiny_muduo::TcpConnection* connection_ptr) {
    // printf("echo_server has a new connection \n");
  }

  // 消息到来时的回调函数。在这里，它从连接中获取消息，然后将消息原样发送回客户端，并打印一条消息到来的信息。
  void MessageCallback(tiny_muduo::TcpConnection* connection_ptr) {
    // std::string message(connection_ptr->Get());
    // // printf("echo_server get message \n");
    // connection_ptr->Send(message);
  }

 private:
  tiny_muduo::EventLoop* loop_;
  tiny_muduo::TCPServer server_;
};

#endif