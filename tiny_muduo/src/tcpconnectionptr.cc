#include "tcpconnectionptr.h"

#include <sys/types.h>
#include <sys/socket.h>

#include "channel.h"
#include "buffer.h"

using namespace tiny_muduo;

// 创建了一个Channel实例，用于管理连接的事件，并设置了读事件的回调函数为TCPConnectionPtr::HandleMessage。
TCPConnectionPtr::TCPConnectionPtr(EventLoop* loop, int connfd)
    : loop_(loop),
      fd_(connfd),
      channel_(new Channel(loop_, fd_)) {
    printf("[Cstr]: TCPConnectionPtr\n");
    channel_->SetReadCallback(std::bind(&TCPConnectionPtr::HandleMessage, this));
    channel_->SetWriteCallback(std::bind(&TCPConnectionPtr::HandleWrite, this));
}

void TCPConnectionPtr::HandleMessage() {
    int read_size = input_buffer_.ReadFd(fd_);
    if(read_size > 0) {
        message_callback_(this, &input_buffer_);
    }
}

void TCPConnectionPtr::HandleWrite() {
    int len = output_buffer_.readablebytes();
    int remaining = len;
    int send_size = send(fd_, output_buffer_.Peek(), remaining, 0);
    remaining -= send_size;

    assert(remaining <= len);
    if(!remaining) {
        channel_->DisableWriting();
    }
}

void TCPConnectionPtr::Send(const char* message, int len) {
    int remaining = len;
    int send_size = 0;
    if(!channel_->IsWriting() && output_buffer_.readablebytes() == 0) {
        send_size = send(fd_, message, len, 0);
        remaining -= send_size;
    }

    assert(remaining <= len);
    if(remaining > 0) {
        output_buffer_.Append(message + send_size, remaining);
        if(!channel_->IsWriting()) {
            channel_->EnableWriting();
        }
    }
}

void TCPConnectionPtr::Send(Buffer* buffer) {
    Send(buffer->Peek(), buffer->readablebytes());
}

void TCPConnectionPtr::Send(const string& message) {
    Send(message.data(), sizeof(message.size()));
}