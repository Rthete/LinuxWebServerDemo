#include "acceptor.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

#include "address.h"
#include "channel.h"

using namespace tiny_muduo;

namespace {
const int kMaxListenNum = 5;
}

// 对监听fd初始化channel，并设置channel的读事件回调函数为NewConnection
Acceptor::Acceptor(EventLoop *loop, const Address &address)
    : loop_(loop),
      listenfd_(socket(PF_INET, SOCK_STREAM, 0)),
      channel_(new Channel(loop, listenfd_)) {
  printf("[Cstr]: Acceptor\n");
  BindListenFd(address);
  channel_->SetReadCallback(std::bind(&Acceptor::NewConnection, this));
}

// 将监听fd绑到地址
void Acceptor::BindListenFd(const Address &addr) {
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(addr.port());
  inet_pton(AF_INET, addr.ip(), &address.sin_addr);
  int ret = bind(listenfd_, (struct sockaddr *)(&address), sizeof(address));
  assert(ret != -1);
}

// 开始监听，并通过EnableReading()把listenfd注册到事件表
void Acceptor::Listen() {
  int ret = listen(listenfd_, kMaxListenNum);
  assert(ret != -1);
  channel_->EnableReading();
}

// 接受新的连接，获取连接的文件描述符，并调用注册的回调函数通知上层代码有新连接到来。
void Acceptor::NewConnection() {
  struct sockaddr_in client;
  socklen_t client_addrlength = sizeof(client);
  int connfd =
      accept(listenfd_, (struct sockaddr *)(&client), &client_addrlength);
  new_connection_callback_(connfd);
}