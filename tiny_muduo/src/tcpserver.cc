#include "tcpserver.h"

#include "acceptor.h"
#include "tcpconnectionptr.h"
#include "eventloopthreadpool.h"

using namespace tiny_muduo;

// 创建了一个Acceptor实例，并设置了新连接到来时的回调函数为TCPServer::NewConnection。
TCPServer::TCPServer(EventLoop* loop, const Address& address)
    : loop_(loop),
      threads_(new EventLoopThreadPool(loop_)),
      acceptor_(new Acceptor(loop, address)) {
    printf("[Cstr]: TCPServer\n");
    acceptor_->SetNewConnectionCallBack(std::bind(&TCPServer::NewConnection, this, _1));
}

TCPServer::~TCPServer() {
    delete threads_;
    delete acceptor_;
}

// 当有新的连接到来时调用的方法。
// 从线程池中获取一个loop，来处理连接
// 在这里，它创建一个TCPConnectionPtr对象表示新连接，并设置连接建立、消息到来时的回调函数，然后将其加入事件循环。
void TCPServer::NewConnection(int connfd) {
    EventLoop* loop = threads_->NextLoop();
    printf("TCPServer NewConnection Arrive Tid:%ld Manage\n", loop->DebugShowTid());

    TCPConnectionPtr* ptr = new TCPConnectionPtr(loop_, connfd);
    ptr->SetConnectionCallback(connection_callback_);
    ptr->SetMessageCallback(message_callback_);
    loop_->RunOneFunc(std::bind(&TCPConnectionPtr::ConnectionEstablished, ptr));
}