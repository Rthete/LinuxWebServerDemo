#ifndef TINY_MODUO_TCPSERVER_H_
#define TINY_MODUO_TCPSERVER_H_

#include "acceptor.h"
#include "callback.h"
#include "eventloop.h"
#include "eventloopthreadpool.h"
#include "noncopyable.h"

namespace tiny_muduo {
class Address;
// class EventLoopThreadPool;

class TCPServer : public NoncopyAble {
 public:
  TCPServer(EventLoop* loop, const Address& address);
  ~TCPServer();

  // 启动TCP服务器，这里的实现是调用Acceptor的Listen方法开始监听连接。
  // 在真实的情况下，可能还需要启动工作线程池等。
  void Start() {
    threads_->StartLoop();
    loop_->RunOneFunc(std::bind(&Acceptor::Listen, acceptor_.get()));
  }
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

  void SetThreadNums(int thread_nums) {
    LOG_INFO("ThreadNums: %d", thread_nums);
    threads_->SetThreadNums(thread_nums);
  }

  void HandleClose(const TcpconnectionPtr& conn);
  void HandleCloseInLoop(const TcpconnectionPtr& ptr);
  void HandleNewConnection(int connfd);

 private:
  typedef std::unordered_map<int, TcpconnectionPtr> ConnectionMap;

  EventLoop* loop_;
  std::unique_ptr<EventLoopThreadPool> threads_;
  std::unique_ptr<Acceptor> acceptor_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  ConnectionMap connections_;
};

}  // namespace tiny_muduo

#endif