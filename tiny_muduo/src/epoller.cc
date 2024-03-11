#include "epoller.h"

#define NDEBUG
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

#include "channel.h"

using namespace tiny_muduo;

Epoller::Epoller()
    : epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kDefaultEvents),
      channels_() {
  printf("[Cstr]: Epoller\n");
}

Epoller::~Epoller() { ::close(epollfd_); }

// 用于epoll_wait，等待事件
void Epoller::Poll(Channels& channels) {
  printf("Epoller Poll start\n");
  int eventnums = EpollWait();
  FillActiveChannels(eventnums, channels);
  printf("Epoller Poll end\n");
}

// 将就绪的事件加入通道
void Epoller::FillActiveChannels(int eventnums, Channels& channels) {
  for (int i = 0; i < eventnums; i++) {
    Channel* ptr = static_cast<Channel*>(events_[i].data.ptr);
    ptr->SetReceivedEvents(events_[i].events);
    channels.emplace_back(ptr);
  }
  // q: 为什么这里要手动扩容
  if (eventnums == static_cast<int>(events_.size())) {
    events_.resize(eventnums * 2);
  }
}

void Epoller::Remove(Channel* channel) {
  int fd = channel->fd();
  ChannelState state = channel->state();
  assert(state == kDeleted || state == kAdded);

  if (state == kAdded) {
    UpdateChannel(EPOLL_CTL_DEL, channel);
  }
  channel->SetChannelState(kNew);
  channels_.erase(fd);
  return;
}

// 更新特定通道的事件监听状态
void Epoller::Update(Channel* channel) {
  int op = 0, events = channel->events();
  ChannelState state = channel->state();
  int fd = channel->fd();

  if (state == kNew || state == kDeleted) {
    if (state == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }
    op = EPOLL_CTL_ADD;
    channel->SetChannelState(kAdded);
  } else {
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    if (events == 0) {
      op = EPOLL_CTL_DEL;
      channel->SetChannelState(kDeleted);
    } else {
      op = EPOLL_CTL_MOD;
    }
  }

  UpdateChannel(op, channel);
}

// 将通道的事件从事件表中添加/删除
void Epoller::UpdateChannel(int operation, Channel* channel) {
  struct epoll_event event;
  memset(&event, '\0', sizeof(event));
  event.events = channel->events();
  event.data.ptr = static_cast<void*>(channel);

  epoll_ctl(epollfd_, operation, channel->fd(), &event);
  return;
}