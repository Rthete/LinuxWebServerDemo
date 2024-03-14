#include "acceptor.h"

#include <arpa/inet.h>
#include <assert.h>
#include <bits/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstring>
#include <functional>

#include "address.h"
#include "channel.h"

using namespace tiny_muduo;

Acceptor::Acceptor(EventLoop* loop, const Address& address)
    : loop_(loop),
      listenfd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                         IPPROTO_TCP)),
      idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
      channel_(new Channel(loop_, listenfd_)) {
  SetSockoptReuseAddr(listenfd_);
  SetSockoptKeepAlive(listenfd_);
  BindListenFd(address);
  channel_->SetReadCallback(std::bind(&Acceptor::NewConnection, this));
}

Acceptor::~Acceptor() {
  channel_->DisableAll();
  loop_->Remove(channel_.get());
  ::close(listenfd_);
}

void Acceptor::BindListenFd(const Address& addr) {
  struct sockaddr_in address;
  bzero((char*)&address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(addr.port());
  int ret = bind(listenfd_, (struct sockaddr*)(&address), sizeof(address));
  assert(ret != -1);
  (void)ret;
}

void Acceptor::Listen() {
  int ret = listen(listenfd_, SOMAXCONN);
  assert(ret != -1);
  (void)ret;
  channel_->EnableReading();
}

void Acceptor::NewConnection() {
  struct sockaddr_in client;
  socklen_t client_addrlength = sizeof(client);
  int connfd = ::accept4(listenfd_, (struct sockaddr*)&client,
                         &client_addrlength, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) {
    if (errno == EMFILE) {
      ::close(idlefd_);
      idlefd_ = ::accept(listenfd_, NULL, NULL);
      ::close(idlefd_);
      idlefd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
    return;
  }
  assert(connfd > 0);
  if (SetSockoptKeepAlive(connfd) == -1) {
    printf("Acceptor::NewConnection SetSockoptKeepAlive failed\n");
    close(connfd);
    return;
  }
  if (SetSockoptTcpNoDelay(connfd) == -1) {
    printf("Acceptor::NewConnection SetSockoptTcpNoDelay failed\n");
    close(connfd);
    return;
  }
  new_connection_callback_(connfd);
}
