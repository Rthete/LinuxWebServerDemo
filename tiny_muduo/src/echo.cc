#include "echo.h"

#include "tcpserver.h"

// 创建了一个TCPServer实例，并设置了连接建立和消息到来时的回调函数
EchoServer::EchoServer(tiny_muduo::EventLoop* loop, const tiny_muduo::Address& listen_addr)
    : loop_(loop),
      server_(loop, listen_addr) {
    printf("[Cstr]: EchoServer\n");
    server_.SetConnectionCallback(std::bind(&EchoServer::ConnectionCallback, this, _1));
    server_.SetMessageCallback(std::bind(&EchoServer::MessageCallback, this, _1));
    server_.SetThreadNums(thread_nums);
}