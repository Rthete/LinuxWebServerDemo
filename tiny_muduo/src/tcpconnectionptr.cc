#include "tcpconnectionptr.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "channel.h"

using namespace tiny_muduo;

// 创建了一个Channel实例，用于管理连接的事件，并设置了读事件的回调函数为TCPConnectionPtr::HandleMessage。
TCPConnectionPtr::TCPConnectionPtr(EventLoop* loop, int connfd)
    : loop_(loop),
      connfd_(connfd),
      channel_(new Channel(loop_, connfd_)) {
    printf("[Cstr]: TCPConnectionPtr\n");
    channel_->SetReadCallback(std::bind(&TCPConnectionPtr::HandleMessage, this));
}

// 处理消息到来事件。当有数据到达时，调用Recv方法接收数据，并调用注册的消息到来回调函数处理消息。
void TCPConnectionPtr::HandleMessage() {
    if(Recv() > 0) {
        message_callback_(this);
    }
}

// 发送消息。将消息拷贝到缓冲区中，然后调用send函数发送消息。
void TCPConnectionPtr::Send(const string& message) {
    strcpy(buff_, message.c_str());
    send(connfd_, (const void*)buff_, sizeof(buff_), 0);
}

// 获取接收到的消息。将缓冲区中的数据拷贝到字符串中，并清空缓冲区。
std::string TCPConnectionPtr::Get() {
    string message(buff_);
    memset(buff_, '\0', sizeof(buff_));
    return message;
}