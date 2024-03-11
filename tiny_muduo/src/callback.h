#ifndef TINY_MODUO_CALLBACK_H_
#define TINY_MODUO_CALLBACK_H_

#include <functional>
#include <memory>

#include "buffer.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace tiny_muduo {
class TcpConnection;
typedef std::shared_ptr<TcpConnection> TcpconnectionPtr;

class Buffer;

typedef std::function<void(const TcpconnectionPtr&, Buffer*)>
    ConnectionCallback;
typedef std::function<void(const TcpconnectionPtr&, Buffer*)> MessageCallback;
typedef std::function<void()> ReadCallback;
typedef std::function<void()> WriteCallback;
typedef std::function<void(const TcpconnectionPtr&)> CloseCallback;
}  // namespace tiny_muduo

#endif