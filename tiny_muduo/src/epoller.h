#ifndef TINY_MUDUO_EPOLLER_H_
#define TINY_MUDUO_EPOLLER_H_

#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "noncopyable.h"

namespace {
const int kDefaultEvents = 16;
}

namespace tiny_muduo {

class Channel;

class Epoller : public NoncopyAble {
 public:
  typedef std::vector<epoll_event> Events;
  typedef std::vector<Channel*> Channels;

  Epoller();
  ~Epoller();

  void Remove(Channel* channel_);
  void Poll(Channels& channels);
  int EpollWait() {
    return epoll_wait(epollfd_, &*events_.begin(),
                      static_cast<int>(events_.size()), -1);
  }
  void FillActiveChannels(int eventnums, Channels& channels);
  void Update(Channel* channel);
  void UpdateChannel(int operation, Channel* channel);

 private:
  typedef std::unordered_map<int, Channel*> ChannelMap;

  int epollfd_;
  Events events_;
  ChannelMap channels_;
};
}  // namespace tiny_muduo

#endif