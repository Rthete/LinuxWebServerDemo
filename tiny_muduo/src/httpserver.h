#ifndef TINY_MUDUO_HTTPSERVER_H_
#define TINY_MUDUO_HTTPSERVER_H_

#include "buffer.h"
#include "httpcontent.h"
#include "httprequest.h"
#include "httpresponse.h"
#include "tcpconnectionptr.h"
#include "tcpserver.h"

using tiny_muduo::HttpStatusCode;

namespace tiny_muduo {
static const int kThreadNums = 8;

class EventLoop;

class HttpServer {
  typedef std::function<void(const HttpRequest &, HttpResponse &)>
      HttpResponseCallback;

 public:
  HttpServer(EventLoop *loop, const Address &address);
  ~HttpServer();

  void Start() { server_.Start(); }

  void HttpDefaultCallback(const HttpRequest &request, HttpResponse &response) {
    response.SetStatusCode(k404NotFound);
    response.SetStatusMessage("Not Found");
    response.SetCloseConnection(true);
  }

  void ConnectionCallback(TCPConnectionPtr *connection) {
    printf("HttpServer has New Connection");
  }
  void MessageCallback(TCPConnectionPtr *connection, Buffer *buffer);
  void SetHttpResponseCallback(const HttpResponseCallback &response_callback) {
    response_callback_ = std::move(response_callback);
  }

  void DealWithRequest(const HttpRequest &request,
                       TCPConnectionPtr *connection);

 private:
  EventLoop *loop_;
  TCPServer server_;

  HttpResponseCallback response_callback_;
};
}  // namespace tiny_muduo

#endif