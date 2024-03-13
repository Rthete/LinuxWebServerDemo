#ifndef TINY_MUDUO_ACCEPTOR_H_
#define TINY_MUDUO_ACCEPTOR_H_

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <functional>
#include <memory>

#include "noncopyable.h"

namespace tiny_muduo {

class EventLoop;
class Address;
class Channel;

class Acceptor : public NoncopyAble {
 public:
  typedef std::function<void(int)> NewConnectionCallback;

  Acceptor(EventLoop *loop, const Address &address);
  ~Acceptor();

  void SetNoneBloking(int fd);
  void BindListenFd(const Address &address);
  void Listen();
  void NewConnection();

  int SetSockoptKeepAlive(int fd) {
    int option_val = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &option_val,
                      static_cast<socklen_t>(sizeof(option_val)));
  }
  int SetSockoptReuseAddr(int fd) {
    int option_val = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &option_val,
                      static_cast<socklen_t>(sizeof(option_val)));
  }
  int SetSockoptTcpNoDelay(int fd) {
    int option_val = 1;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &option_val,
                      static_cast<socklen_t>(sizeof(option_val)));
  }
  void SetNewConnectionCallBack(const NewConnectionCallback &callback) {
    new_connection_callback_ = callback;
  }
  void SetNewConnectionCallBack(NewConnectionCallback &&callback) {
    new_connection_callback_ = std::move(callback);
  }

 private:
  EventLoop *loop_;
  int listenfd_;
  int idlefd_;  // 防止fd耗尽导致的崩溃
  std::unique_ptr<Channel> channel_;

  NewConnectionCallback new_connection_callback_;
};
}  // namespace tiny_muduo

#endif