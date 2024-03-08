#ifndef TINY_MUDUO_HTTPSTATE_H_
#define TINY_MUDUO_HTTPSTATE_H_

namespace tiny_muduo {
// 标识HTTP请求解析的阶段
enum HttpRequestParseState {
  kParseRequestLine,  // 请求的方法、请求的URL以及HTTP协议的版本号
  kParseHeaders,  // 客户端向服务器发送的各种信息，例如用户代理、接受的内容类型等
  kParseBody,  // 客户端发送给服务器的数据，例如表单数据、JSON数据等
  kParseGotCompleteRequest,
  kParseErrno,
};
}  // namespace tiny_muduo

#endif
