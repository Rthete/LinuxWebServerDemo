#ifndef TINY_MODUO_TCPCONNECTIONPTR_H_
#define TINY_MODUO_TCPCONNECTIONPTR_H_

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>

#include "callback.h"
#include "channel.h"

using std::string;

namespace tiny_muduo
{
class EventLoop;

class TCPConnectionPtr {
public:
    TCPConnectionPtr(EventLoop* loop, int connfd);
    ~TCPConnectionPtr();

    // 设置连接建立时的回调函数
    void SetConnectionCallback(const ConnectionCallback& callback) {
        connection_callback_ = callback;
    }
    // 设置消息到来时的回调函数
    void SetMessageCallback(const MessageCallback& callback) {
        message_callback_ = callback;
    }
    // 表示连接建立，启用读事件监听，并调用连接建立的回调函数通知上层代码连接已建立。
    void ConnectionEstablished() {
        channel_->EnableReading();
        connection_callback_(this);
    }

    void HandleMessage();
    void Send(const string& str);
    string Get();

private:
    int Recv() {
        memset(buff_, '\0', sizeof(buff_));
        int ret = recv(connfd_, buff_, 100, 0);
        return ret;
    }
    
    EventLoop* loop_;
    int connfd_;
    Channel* channel_;
    char buff_[100] = {0};

    ConnectionCallback connection_callback_;
    MessageCallback message_callback_;
};

} // namespace tiny_muduo

#endif