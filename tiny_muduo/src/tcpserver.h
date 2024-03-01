#ifndef TINY_MODUO_TCPSERVER_H_
#define TINY_MODUO_TCPSERVER_H_

#include "callback.h"
#include "eventloop.h"
#include "acceptor.h"

namespace tiny_muduo
{
class Address;
// class EventLoopThreadPool;

class TCPServer {
public:
    TCPServer(EventLoop* loop, const Address& address);
    ~TCPServer();

    // 启动TCP服务器，这里的实现是调用Acceptor的Listen方法开始监听连接。
    // 在真实的情况下，可能还需要启动工作线程池等。
    void Start() {
        // threads.Start();
        loop_->RunOneFunc(std::bind(&Acceptor::Listen, acceptor_));
    }
    // 设置连接建立时的回调函数
    void SetConnectionCallback(const ConnectionCallback& callback) {
        connection_callback_ = callback;
    }
    // 设置消息到来时的回调函数
    void SetMessageCallback(const MessageCallback& callback) {
        message_callback_ = callback;
    }
    
    void NewConnection(int connfd);

private:
    EventLoop* loop_;
    // EventLoopThreadPool* threads_;
    Acceptor* acceptor_;

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
};

} // namespace tiny_muduo


#endif