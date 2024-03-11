#include "tcpconnection.h"

#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "channel.h"

using std::string;

using namespace tiny_muduo;

// 创建了一个Channel实例，用于管理连接的事件，并设置了读事件的回调函数为TcpConnection::HandleMessage。
TcpConnection::TcpConnection(EventLoop* loop, int connfd)
    : loop_(loop),
      fd_(connfd),
      state_(kDisconnected),
      channel_(new Channel(loop_, fd_)),
      shutdown_state_(false) {
  printf("[Cstr]: TcpConnection\n");
  channel_->SetReadCallback(std::bind(&TcpConnection::HandleMessage, this));
  channel_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
}

TcpConnection::~TcpConnection() {
  // printf("TcpConnection::~TcpConnection destructor\n");
  ::close(fd_);
}

void TcpConnection::ConnectionDestructor() {
  if (state_ == kConnected) {
    state_ = kDisconnected;
    channel_->DisableAll();
  }
  loop_->Remove(channel_.get());
}

void TcpConnection::HandleClose() {
  state_ = kDisconnected;
  channel_->DisableAll();
  TcpconnectionPtr guard(shared_from_this());
  close_callback_(guard);
}

void TcpConnection::HandleMessage() {
  int read_size = input_buffer_.ReadFd(fd_);
  if (read_size > 0) {
    message_callback_(shared_from_this(), &input_buffer_);
  } else if (read_size == 0) {
    HandleClose();
  }
}

void TcpConnection::HandleWrite() {
  int len = output_buffer_.readablebytes();
  int remaining = len;
  int send_size = ::write(fd_, output_buffer_.Peek(), remaining);
  remaining -= send_size;

  assert(remaining <= len);
  if (!remaining) {
    channel_->DisableWriting();
  }
}

void TcpConnection::Send(const char* message, int len) {
  int remaining = len;
  int send_size = 0;
  if (!channel_->IsWriting() && output_buffer_.readablebytes() == 0) {
    send_size = ::write(fd_, message, len);
    remaining -= send_size;
  }

  assert(remaining <= len);
  if (remaining > 0) {
    output_buffer_.Append(message + send_size, remaining);
    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::Shutdown() {
  if (!channel_->IsWriting()) {
    ::shutdown(fd_, SHUT_WR);
    shutdown_state_ = true;
  }
}

void TcpConnection::Send(Buffer* buffer) {
  Send(buffer->Peek(), buffer->readablebytes());
  buffer->RetrieveAll();
}

void TcpConnection::Send(const string& message) {
  Send(message.data(), sizeof(message.size()));
}