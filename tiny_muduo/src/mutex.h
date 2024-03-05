#ifndef TINY_MODUO_MUTEX_H_
#define TINY_MODUO_MUTEX_H_

#include <pthread.h>

namespace tiny_muduo
{
class MutexLock {


public:
    MutexLock() {
        pthread_mutex_init(&mutex_, nullptr);
    }
    ~MutexLock() {
        pthread_mutex_destroy(&mutex_);
    }

    bool Lock() {
        return pthread_mutex_lock(&mutex_) == 0;
    }

    bool Unlock() {
        return pthread_mutex_unlock(&mutex_) == 0;
    }

    pthread_mutex_t* mutex() { return &mutex_; };

private:
    pthread_mutex_t mutex_;
};

class MutexLockGuard {
public:
    MutexLockGuard(MutexLock& mutex) : mutex_(mutex) {
        mutex_.Lock();
    }
    ~MutexLockGuard() {
        mutex_.Unlock();
    }

private:
    MutexLock& mutex_;
};
} // namespace tiny_muduo



#endif