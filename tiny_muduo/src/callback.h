#ifndef TINY_MODUO_CALLBACK_H_
#define TINY_MODUO_CALLBACK_H_

#include <functional>

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace tiny_muduo {
    class TCPConnectionPtr;
    typedef std::function<void(TCPConnectionPtr*)> ConnectionCallback;
    typedef std::function<void(TCPConnectionPtr*)> MessageCallback;
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCallback;
}
#endif