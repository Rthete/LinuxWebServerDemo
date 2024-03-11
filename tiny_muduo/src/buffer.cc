#include "buffer.h"

#include <sys/uio.h>

using namespace tiny_muduo;

int Buffer::ReadFd(int fd) {
  // 64K栈空间，会随着函数栈帧回退，内存自动回收
  char extrabuff[65536] = {0};
  struct iovec iv[2];
  const int writable = writablebytes();
  // 第一块缓冲区
  iv[0].iov_base = beginwrite();
  iv[0].iov_len = writable;
  // 第二块缓冲区
  iv[1].iov_base = extrabuff;
  iv[1].iov_len = sizeof(extrabuff);

  // 如果Buffer有65536字节的空闲空间，就不使用栈上的缓冲区
  // 如果不够65536字节，就使用栈上的缓冲区，即readv一次最多读取65536字节数据
  const int iovcnt = (writable < static_cast<int>(sizeof(extrabuff)) ? 2 : 1);

  /** readv
   *    readv可以在不连续的多个地址写入同一个fd上读取的数据
   *    ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
   *      fd：读取数据的文件描述符
   *      iov：封装了缓冲区地址和可写空间大小的结构体
   *      iovcnt：缓冲区个数
   */
  int readn = readv(fd, iv, iovcnt);

  if (readn < 0) {
    printf("Buffer::ReadFd readn < 0 SYS_ERR\n");
  } else if (readn <= writable) {
    // 读取的数据n小于Buffer底层的可写空间
    // readv会直接把数据存放在begin() + writerIndex_
    write_index_ += readn;
  } else {
    // Buffer底层的可写空间不够存放n字节数据
    // extrabuff中有部分数据（n - writable）
    write_index_ = buffer_.size();
    // 从extrabuff里读取 n - writable 字节的数据存入Buffer底层的缓冲区
    Append(extrabuff, readn - writable);
  }

  return readn;
}