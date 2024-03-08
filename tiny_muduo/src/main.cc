#include "address.h"
#include "eventloop.h"
#include "httprequest.h"
#include "httpresponsefile.h"
#include "httpserver.h"

using namespace tiny_muduo;

using tiny_muduo::HttpStatusCode;
using tiny_muduo::Method;

void HttpResponseCallback(const HttpRequest& request, HttpResponse& response) {
  if (request.method() != kGet) {
    response.SetStatusCode(k400BadRequest);
    response.SetStatusMessage("Bad Request");
    response.SetCloseConnection(true);
    return;
  }

  {
    const string& path = request.path();
    if (path == "/") {
      response.SetStatusCode(k200OK);
      response.SetBodyType("text/html");
      response.SetBody(love6_website);
    } else if (path == "/hello") {
      response.SetStatusCode(k200OK);
      response.SetBodyType("text/plain");
      response.SetBody("Hello, world!\n");
    } else if (path == "/favicon") {
      response.SetStatusCode(k200OK);
      response.SetBodyType("image/png");
      response.SetBody(string(favicon, sizeof(favicon)));
    } else {
      response.SetStatusCode(k404NotFound);
      response.SetStatusMessage("Not Found");
      response.SetBody("Sorry, Not Found\n");
      response.SetCloseConnection(true);
      return;
    }
  }
}

int main() {
  // 创建事件循环
  tiny_muduo::EventLoop loop;

  // 设置监听地址和端口
  tiny_muduo::Address listen_addr("2022");

  // // 创建Echo服务器，绑定回调函数
  // EchoServer echo_server(&loop, listen_addr);
  // // 启动Echo服务器，调用listen
  // echo_server.Start();

  // 创建HttpServer
  HttpServer server(&loop, listen_addr);
  server.SetHttpResponseCallback(HttpResponseCallback);
  server.Start();

  // 运行事件循环
  loop.Loop();

  return 0;
}
