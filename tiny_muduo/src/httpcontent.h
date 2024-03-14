#ifndef TINY_MUDUO_HTTPCONTENT_H_
#define TINY_MUDUO_HTTPCONTENT_H_

#include "buffer.h"
#include "httprequest.h"
#include "httpstate.h"

namespace tiny_muduo {
enum HttpRequestParseLine { kLineOK, kLineMore, kLineErrno };

class HttpContent {
 public:
  HttpContent();
  ~HttpContent();

  void ParseLine(Buffer* buffer);
  bool ParseContent(Buffer* buffer);
  bool GetCompleteRequest() { return parse_state_ == kParseGotCompleteRequest; }

  const HttpRequest& request() { return request_; }
  void ResetContentState() {
    // HttpRequest tmp;
    // request_.Swap(tmp);
    parse_state_ = kParseRequestLine;
  }

 private:
  HttpRequest request_;
  HttpRequestParseState parse_state_;
};
}  // namespace tiny_muduo

#endif