#ifndef TINY_MUDUO_BUFFER_H_
#define TINY_MUDUO_BUFFER_H_

#include <vector>
#include <algorithm>
#include <string>
#include <assert.h>

using std::string;

namespace tiny_muduo
{
static const int kPrePendIndex = 8;

class Buffer {
public:
    Buffer(): buffer_(1024), read_index_(kPrePendIndex), write_index_(kPrePendIndex) {}
    ~Buffer() {}

    int ReadFd(int fd);

    // 指向buffer头部
    char* begin() { return &*buffer_.begin(); };
    const char* begin() const { return &*buffer_.begin(); };

    // 指向待读取区域头部
    char* beginread() { return begin() + read_index_; }
    const char* beginread() const { return begin() + read_index_; }

    // 指向待写区域头部
    char* beginwrite() { return begin() + write_index_; }
    const char* beginwrite() const { return begin() + write_index_; }

    void Append(const char* message, int len) {
        MakeSureEnoughStorage(len);
        std::copy(message, message + len, beginwrite());
        write_index_ += len;
    }
    void Append(const string& message) {
        Append(message.data(), message.size());
    }

    void Retrieve(int len) {
        // len就是应用程序从Buffer缓冲区读取的数据长度
        if(len < readablebytes()) {
            // 可读数据没有读完
            read_index_ = read_index_ + len;
        } else {
            // 可读数据读完了，readerIndex_和writerIndex_都要复位
            RetrieveAll();
        }
    }

    void RetrieveUntilIndex(const char* index) {
        assert(beginwrite() >= index);
        read_index_ += index - beginread();
    }

    void RetrieveAll() {
        write_index_ = kPrePendIndex;
        read_index_ = write_index_;
    }

    string RetrieveAsString(int len) {
        assert(read_index_ + len <= write_index_);
        string ret = std::move(PeekAsString(len));
        Retrieve(len);
        return ret;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* Peek() const {
        return beginread();
    }

    char* Peek() {
        return beginread();
    }

    string PeekAsString(int len) {
        return string(beginread(), beginread() + len);
    }

    string PeekAsString() {
        return string(beginread(), beginwrite());
    }

    // 待读取数据长度：[readerIndex_, writerIndex_]
    int readablebytes() const { return write_index_ - read_index_; }
    // 可写空闲大小
    int writablebytes() const { return buffer_.capacity() - write_index_; }
    int prependablebytes() const { return read_index_; }

    void MakeSureEnoughStorage(int len) {
        if(writablebytes() >= len) return;
        if(writablebytes() + prependablebytes() >= kPrePendIndex + len) {
            // 如果空闲空间足够存放len字节的数据，就把未读取的数据统一往前移
            // 把[readerIndex_, writerIndex_]整体搬到kPrePendIndex
            std::copy(beginread(), beginwrite(), begin() + kPrePendIndex);
            // writerIndex_指向待读取数据的末尾
            write_index_ = kPrePendIndex + readablebytes();
            read_index_ = kPrePendIndex;
        } else {
            // 直接在writerIndex_后面再扩大len的空间(利用vector的特性)
            buffer_.resize(buffer_.size() + len);
        }
    }

private:
    std::vector<char> buffer_;
    // 应用程序从readerIndex指向的位置开始读缓冲区, 读完后向后移动len（retrieve方法）
    int read_index_;
    // 应用程序从writerIndex指向的位置开始写缓冲区，写完后writerIndex向后移动len（append方法）
    int write_index_;
};

} // namespace tiny_muduo



#endif