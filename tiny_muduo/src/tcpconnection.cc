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
  } else {
    printf("TcpConnection::HandleMessage Read SYS_Err\n");
  }
}

void TcpConnection::HandleWrite() {
  if (channel_->IsWriting()) {
    int len = output_buffer_.readablebytes();
    int remaining = len;
    int send_size = ::write(fd_, output_buffer_.Peek(), remaining);
    if (send_size < 0) {
      assert(send_size > 0);
      if (errno != EWOULDBLOCK) {
        printf("TcpConnection::HandleWrite Write SYS_ERR\n");
      }
      return;
    }
    remaining -= send_size;
    output_buffer_.Retrieve(send_size);

    assert(remaining <= len);
    if (!output_buffer_.readablebytes()) {
      channel_->DisableWriting();
    }
  }
}

void TcpConnection::Send(const char* message, int len) {
  int remaining = len;
  int send_size = 0;
  if (!channel_->IsWriting() && output_buffer_.readablebytes() == 0) {
    send_size = ::write(fd_, message, len);
    if (send_size >= 0) {
      remaining -= send_size;
    } else {
      if (errno != EWOULDBLOCK) {
        printf("TcpConnection::Send Write SYS_ERR\n");
      }
      return;
    }
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
  shutdown_state_ = true;
  if (!channel_->IsWriting()) {
    int ret = ::shutdown(fd_, SHUT_WR);
    if (ret < 0) {
      printf("TcpConnection::Shutdown shutdown SYS_ERR\n");
    }
  }
}

void TcpConnection::Send(Buffer* buffer) {
  if (state_ == kDisconnected) return;
  Send(buffer->Peek(), buffer->readablebytes());
  buffer->RetrieveAll();
}

void TcpConnection::Send(const string& message) {
  if (state_ == kDisconnected) return;
  Send(message.data(), message.size());
}