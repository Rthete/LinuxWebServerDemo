#ifndef TINY_MUDUO_ACCEPTOR_H_
#define TINY_MUDUO_ACCEPTOR_H_

#include <functional>
#include <memory>

namespace tiny_muduo {

class EventLoop;
class Address;
class Channel;

class Acceptor {
 public:
  typedef std::function<void(int)> NewConnectionCallback;

  Acceptor(EventLoop *loop, const Address &address);
  ~Acceptor();

  void SetNoneBloking(int fd);
  void BindListenFd(const Address &address);
  void Listen();
  void NewConnection();
  void SetNewConnectionCallBack(const NewConnectionCallback &callback) {
    new_connection_callback_ = callback;
  }

 private:
  EventLoop *loop_;
  int listenfd_;
  std::unique_ptr<Channel> channel_;

  NewConnectionCallback new_connection_callback_;
};
}  // namespace tiny_muduo

#endif