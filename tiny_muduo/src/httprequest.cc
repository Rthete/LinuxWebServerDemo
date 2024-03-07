#include "httprequest.h"

#include <algorithm>

#include "httpstate.h"

using namespace tiny_muduo;
using tiny_muduo::Method;
using tiny_muduo::HttpRequestParseState;

HttpRequest::HttpRequest() {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::ParseRequestMethod(const char* start, const char* end) {
    string method(start, end);
    bool has_method = true;
    if (method == "GET") {
        method_ = kGet;
    } else if (method == "POST") {
        method_ = kPost;
    } else if (method == "PUT") {
        method_ = kPut;
    } else if (method == "DELETE") {
        method_ = kDelete;
    } else if (method == "TRACE") {
        method_ = kTrace;
    } else if (method == "OPTIONS") {
        method_ = kOptions;
    } else if (method == "CONNECT") {
        method_ = kConnect;
    } else if (method == "PATCH") {
        method_ = kPatch;
    } else {
        has_method = false;
    }
    return has_method;
}

// Request Line 形如：GET / HTTP/1.1
void HttpRequest::ParseRequestLine(const char* start, const char* end,
                    HttpRequestParseState& state) {
    {
        const char* space = std::find(start, end, ' ');
        if(space == end) {
            state = kParseErrno;
            return;
        }

        if(!ParseRequestMethod(start, space)) {
            state = kParseErrno;
            return;
        }
        start = space + 1;
    }

    {
        const char* space = std::find(start, end, ' ');
        if(space == end) {
            state = kParseErrno;
            return;
        }

        const char* query = std::find(start, end, '?');
        if(query != end) {
            path_ = std::move(string(start, query));
            query_ = std::move(string(query + 1, space));
        } else {
            path_ = std::move(string(start, space));
        }
        start = space + 1;
    }

    {
        // 在request字符串中寻找形如"HTTP/1."的子串
        const int httplen = sizeof(http) / sizeof(char) - 1;
        const char* httpindex = std::search(start, end, http, http + httplen);
        if(httpindex == end) {
            state = kParseErrno;
            return;
        }

        // 解析是1.0还是1.1
        const char chr = *(httpindex + httplen);
        if(httpindex + httplen + 1 == end && (chr == '1' || chr == '0')) {
            if(chr == '1') {
                version_ = kHttp11;
            } else {
                version_ = kHttp10;
            }
        } else {
            state = kParseErrno;
            return;
        }
    }
    // 解析完request line后将状态置为解析headers
    state = kParseHeaders;
}

// 解析headers中的键值对，并存入headers_中
// 例如: Host: www.example.com
// 得到headers{Host, www.example.com}
void HttpRequest::ParseHeaders(const char* start, const char* end,
                        HttpRequestParseState& state) {
    // 由于暂未处理body，在headers解析完后将状态标识为解析结束
    if(start == end && *start == '\r' && *(start + 1) == '\n') {
        state = kParseGotCompleteRequest;
        return;
    }

    const char* colon = std::find(start, end, ':');
    if(colon == end) {
        state = kParseErrno;
        return;
    }

    const char* valid = colon + 1;
    while(*(valid++) != ' ') {}
    headers_[std::move(string(start, colon))] = std::move(string(colon + 1, valid));
    return;
}

// TODO: 暂未实现解析请求体
void HttpRequest::ParseBody(const char* start, const char* end,
                            HttpRequestParseState& state) {
}