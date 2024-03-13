#ifndef TINY_MUDUO_NONCOPYABLE_H_
#define TINY_MUDUO_NONCOPYABLE_H_

namespace tiny_muduo {
class NoncopyAble {
 protected:
  NoncopyAble() {}
  ~NoncopyAble() {}

 private:
  NoncopyAble(const NoncopyAble&) = delete;
  NoncopyAble& operator=(const NoncopyAble&) = delete;
};
}  // namespace tiny_muduo

#endif