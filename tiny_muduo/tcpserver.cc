#include "tcpserver.h"

#include "acceptor.h"
#include "tcpconnectionptr.h"

using namespace tiny_muduo;

// 创建了一个Acceptor实例，并设置了新连接到来时的回调函数为TCPServer::NewConnection。
TCPServer::TCPServer(EventLoop* loop, const Address& address)
    : loop_(loop),
    //   threads_(nullptr),
      acceptor_(new Acceptor(loop, address)) {
    acceptor_->SetNewConnectionCallBack(std::bind(&TCPServer::NewConnection, this, _1));
}

TCPServer::~TCPServer() {
    // delete threads_;
    delete acceptor_;
}

// 当有新的连接到来时调用的方法。
// 在这里，它创建一个TCPConnectionPtr对象表示新连接，并设置连接建立、消息到来时的回调函数，然后将其加入事件循环。
void TCPServer::NewConnection(int connfd) {
    TCPConnectionPtr* ptr = new TCPConnectionPtr(loop_, connfd);
    ptr->SetConnectionCallback(connection_callback_);
    ptr->SetMessageCallback(message_callback_);
    loop_->RunOneFunc(std::bind(&TCPConnectionPtr::ConnectionEstablished, ptr));
}