#include "tcpserver.h"

#include "acceptor.h"
#include "eventloopthreadpool.h"
#include "tcpconnection.h"

using namespace tiny_muduo;

// 创建了一个Acceptor实例，并设置了新连接到来时的回调函数为TCPServer::NewConnection。
TCPServer::TCPServer(EventLoop* loop, const Address& address)
    : loop_(loop),
      threads_(new EventLoopThreadPool(loop_)),
      acceptor_(new Acceptor(loop_, address)) {
  printf("[Cstr]: TCPServer\n");
  acceptor_->SetNewConnectionCallBack(
      std::bind(&TCPServer::HandleNewConnection, this, _1));
}

TCPServer::~TCPServer() {
  for (auto& pair : connections_) {
    TcpconnectionPtr ptr(pair.second);
    pair.second.reset();
    ptr->loop()->RunOneFunc(
        std::bind(&TcpConnection::ConnectionDestructor, ptr));
  }
}

void TCPServer::HandleClose(const TcpconnectionPtr& ptr) {
  loop_->QueueOneFunc(std::bind(&TCPServer::HandleCloseInLoop, this, ptr));
}

void TCPServer::HandleCloseInLoop(const TcpconnectionPtr& ptr) {
  assert(connections_.find(ptr->fd()) != connections_.end());
  connections_.erase(connections_.find(ptr->fd()));
  EventLoop* loop = ptr->loop();
  loop->QueueOneFunc(std::bind(&TcpConnection::ConnectionDestructor, ptr));
}

// 当有新的连接到来时调用的方法。
// 从线程池中获取一个loop，来处理连接
// 在这里，它创建一个TcpConnection对象表示新连接，并设置连接建立、消息到来时的回调函数，然后将其加入事件循环。
void TCPServer::HandleNewConnection(int connfd) {
  EventLoop* loop = threads_->NextLoop();
  TcpconnectionPtr ptr(new TcpConnection(loop, connfd));
  connections_[connfd] = ptr;
  ptr->SetConnectionCallback(connection_callback_);
  ptr->SetMessageCallback(message_callback_);
  ptr->SetCloseCallback(std::bind(&TCPServer::HandleClose, this, _1));
  loop->RunOneFunc(std::bind(&TcpConnection::ConnectionEstablished, ptr));
}