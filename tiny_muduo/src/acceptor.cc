#include "acceptor.h"

#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <cstring>

#include "address.h"
#include "channel.h"

using namespace tiny_muduo;

// 对监听fd初始化channel，并设置channel的读事件回调函数为NewConnection
Acceptor::Acceptor(EventLoop *loop, const Address &address)
    : loop_(loop),
      listenfd_(::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                         IPPROTO_TCP)),
      idlefd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)),
      channel_(new Channel(loop, listenfd_)) {
  printf("[Cstr]: Acceptor\n");
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

void Acceptor::SetNoneBloking(int fd) {
  int old_state = fcntl(fd, F_GETFL);
  int new_state = old_state | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_state);
  (void)new_state;
}

// 将监听fd绑到地址
void Acceptor::BindListenFd(const Address &addr) {
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(addr.port());
  inet_pton(AF_INET, addr.ip(), &address.sin_addr);
  int ret = bind(listenfd_, (struct sockaddr *)(&address), sizeof(address));
  assert(ret != -1);
  (void)ret;  // 避免编译器对未使用变量发出警告
}

// 开始监听，并通过EnableReading()把listenfd注册到事件表
void Acceptor::Listen() {
  int ret = listen(listenfd_, SOMAXCONN);
  assert(ret != -1);
  (void)ret;
  channel_->EnableReading();
}

// 接受新的连接，获取连接的文件描述符，并调用注册的回调函数通知上层代码有新连接到来。
void Acceptor::NewConnection() {
  struct sockaddr_in client;
  socklen_t client_addrlength = sizeof(client);
  int connfd = accept4(listenfd_, (struct sockaddr *)(&client),
                       &client_addrlength, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0) {
    // 文件描述符耗尽的错误码
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