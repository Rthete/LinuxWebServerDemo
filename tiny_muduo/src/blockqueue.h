#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <assert.h>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>
template <class T>
class BlockDeque {
 public:
  explicit BlockDeque(size_t MaxCapacity = 1000);
  ~BlockDeque();

  void clear();
  bool empty();
  bool full();
  void close();
  size_t size();
  size_t capacity();
  T front();
  T back();
  void push_back(const T &item);
  void push_front(const T &item);
  bool pop(T &item);
  bool pop(T &item, int timeout);
  void flush();

 private:
  std::deque<T> deq_;
  size_t capacity_;
  std::mutex mtx_;
  bool isClose_;
  std::condition_variable condConsumer_;
  std::condition_variable condProducer_;
};

template <class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity) : capacity_(MaxCapacity) {
  assert(MaxCapacity > 0);
  isClose_ = false;
}

template <class T>
void BlockDeque<T>::close() {
  {
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
    isClose_ = true;
  }
  condProducer_.notify_all();
  condConsumer_.notify_all();
}

template <class T>
void BlockDeque<T>::flush() {
  condConsumer_.notify_one();
};

template <class T>
void BlockDeque<T>::clear() {
  std::lock_guard<std::mutex> locker(mtx_);
  deq_.clear();
}

template <class T>
T BlockDeque<T>::front() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.front();
}

template <class T>
T BlockDeque<T>::back() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.back();
}

template <class T>
size_t BlockDeque<T>::size() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size();
}

template <class T>
size_t BlockDeque<T>::capacity() {
  std::lock_guard<std::mutex> locker(mtx_);
  return capacity_;
}

template <class T>
void BlockDeque<T>::push_back(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    // 等待生产者条件变量的发生，并在等待期间释放 mtx_
    // 互斥锁。等待期间，当前线程会被阻塞，直到其他线程通过调用
    // condProducer_.notify_one() 或 condProducer_.notify_all()
    // 来通知条件变量的发生，并且重新获取了 mtx_ 互斥锁。
    condProducer_.wait(locker);
  }
  deq_.push_back(item);
  condConsumer_.notify_one();
}

template <class T>
void BlockDeque<T>::push_front(const T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.size() >= capacity_) {
    condProducer_.wait(locker);
  }
  deq_.push_front(item);
  condConsumer_.notify_one();
}

template <class T>
bool BlockDeque<T>::empty() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.empty();
}

template <class T>
bool BlockDeque<T>::full() {
  std::lock_guard<std::mutex> locker(mtx_);
  return deq_.size() >= capacity_;
}

template <class T>
bool BlockDeque<T>::pop(T &item) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    condConsumer_.wait(locker);
    if (isClose_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  condProducer_.notify_one();
  return true;
}

template <class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
  std::unique_lock<std::mutex> locker(mtx_);
  while (deq_.empty()) {
    if (condConsumer_.wait_for(locker, std::chrono::seconds(timeout)) ==
        std::cv_status::timeout) {
      return false;
    }
    if (isClose_) {
      return false;
    }
  }
  item = deq_.front();
  deq_.pop_front();
  condProducer_.notify_one();
  return true;
}

#endif