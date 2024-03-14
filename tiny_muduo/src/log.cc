#include "log.h"

#include <bits/types/time_t.h>
#include <sys/select.h>

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

using namespace std;

Log::Log() {
  lineCount_ = 0;
  isAsync_ = false;
  writeThread_ = nullptr;
  deque_ = nullptr;
  toDay_ = 0;
  fp_ = nullptr;
}

Log::~Log() {
  if (writeThread_ && writeThread_->joinable()) {
    while (!deque_->empty()) {
      deque_->flush();
    };
    deque_->close();
    writeThread_->join();
  }
  if (fp_) {
    lock_guard<mutex> locker(mtx_);
    flush();
    fclose(fp_);
  }
}

int Log::GetLevel() {
  lock_guard<mutex> locker(mtx_);
  return level_;
}

void Log::SetLevel(int level) {
  lock_guard<mutex> locker(mtx_);
  level_ = level;
}

void Log::init(int level = 1, const char* path, const char* suffix,
               int maxQueueSize) {
  isOpen_ = true;
  level_ = level;
  //如果设置了max_queue_size,则设置为异步日志，创建阻塞队列
  if (maxQueueSize > 0) {
    isAsync_ = true;
    if (!deque_) {
      unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
      deque_ = std::move(newDeque);

      std::unique_ptr<std::thread> NewThread(new thread(FlushLogThread));
      writeThread_ = move(NewThread);
    }
  } else {
    isAsync_ = false;
  }
  lineCount_ = 0;
  time_t timer = time(nullptr);
  /** tm 和 localtime()
   * localtime()返回的是一个静态分配的结构体指针，它指向的内存空间可能会在后续调用
   * localtime()时被覆盖。为了避免这种情况，我们先将其内容拷贝到一个新的结构体变量
   * t 中，这样就可以确保 t 不会受到后续调用 localtime() 函数的影响。
   */
  struct tm* sysTime = localtime(&timer);
  struct tm t = *sysTime;
  path_ = path;
  suffix_ = suffix;
  char fileName[LOG_NAME_LEN] = {0};
  // 生成文件名格式：path_/YYYY_MM_DDsuffix_
  snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_,
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
  toDay_ = t.tm_mday;

  {
    lock_guard<mutex> locker(mtx_);
    buff_.RetrieveAll();
    if (fp_) {
      flush();
      fclose(fp_);
    }

    fp_ = fopen(fileName, "a");
    if (fp_ == nullptr) {
      mkdir(path_, 0777);
      fp_ = fopen(fileName, "a");
    }
    assert(fp_ != nullptr);
  }
}

void Log::write(int level, const char* format, ...) {
  struct timeval now = {0, 0};
  gettimeofday(&now, nullptr);
  time_t tSec = now.tv_sec;
  struct tm* sysTime = localtime(&tSec);
  struct tm t = *sysTime;
  va_list vaList;

  // 需要建立新日志文件的情况
  if (toDay_ != t.tm_mday || (lineCount_ && (lineCount_ % MAX_LINES == 0))) {
    unique_lock<mutex> locker(mtx_);
    locker.unlock();

    char newFile[LOG_NAME_LEN];
    char tail[36] = {0};
    // tail: 2024_03_14
    snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1,
             t.tm_mday);

    if (toDay_ != t.tm_mday) {
      // 如果是新的一天，建立新的当天日期日志
      // newFile: path_/2024_03_14.log
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
      toDay_ = t.tm_mday;
      lineCount_ = 0;
    } else {
      // 如果是同一天，但当前日志文件达到最大行数
      snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail,
               (lineCount_ / MAX_LINES), suffix_);
    }

    locker.lock();
    flush();
    fclose(fp_);
    fp_ = fopen(newFile, "a");
    assert(fp_ != nullptr);
  }

  {
    unique_lock<mutex> locker(mtx_);
    lineCount_++;
    /** 为什么使用snprintf，而不是直接给字符数组赋值？
     * 如果格式化后的字符串长度超过了 size - 1，则会截断超出部分，
     * 并在末尾添加 \0，以确保结果是一个以 \0 结尾的 C 字符串。
     * 动态地构造字符串，并且能够安全地将格式化的字符串写入到指定的缓冲区中，
     * 避免了数组越界的问题。
     */
    int n =
        snprintf(buff_.beginwrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
                 t.tm_sec, now.tv_usec);

    buff_.HasWritten(n);
    AppendLogLevelTitle_(level);

    /** void va_start(va_list ap, last_arg);
     *  初始化 ap，使其指向可变参数列表的起始位置
     */
    va_start(vaList, format);
    int m =
        vsnprintf(buff_.beginwrite(), buff_.writablebytes(), format, vaList);
    va_end(vaList);  // 清理可变参数列表

    buff_.HasWritten(m);
    buff_.Append("\n\0", 2);

    // 将缓冲区内容入队（生产）
    if (isAsync_ && deque_ && !deque_->full()) {
      deque_->push_back(buff_.RetrieveAllAsString());
    } else {
      // 同步模式
      fputs(buff_.Peek(), fp_);
    }
    buff_.RetrieveAll();
  }
}

void Log::AppendLogLevelTitle_(int level) {
  switch (level) {
    case 0:
      buff_.Append("[debug]: ", 9);
      break;
    case 1:
      buff_.Append("[info] : ", 9);
      break;
    case 2:
      buff_.Append("[warn] : ", 9);
      break;
    case 3:
      buff_.Append("[error]: ", 9);
      break;
    default:
      buff_.Append("[info] : ", 9);
      break;
  }
}

void Log::flush() {
  if (isAsync_) {
    deque_->flush();
  }
  fflush(fp_);
}

void Log::AsyncWrite_() {
  string str = "";
  // 取出阻塞队列头（消费）
  while (deque_->pop(str)) {
    lock_guard<mutex> locker(mtx_);
    fputs(str.c_str(), fp_);
  }
}

Log* Log::Instance() {
  // 利用C++11静态的特性实现单例
  static Log inst;
  return &inst;
}

void Log::FlushLogThread() { Log::Instance()->AsyncWrite_(); }