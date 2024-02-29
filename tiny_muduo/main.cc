#include "eventloop.h"
#include "echo.h"
#include "address.h"

int main() {
    // 创建事件循环
    tiny_muduo::EventLoop loop;

    // 监听地址和端口
    tiny_muduo::Address listen_addr("2022");

    // 创建Echo服务器
    EchoServer echo_server(&loop, listen_addr);

    // 启动Echo服务器
    echo_server.Start();

    // 运行事件循环
    loop.Loop();

    return 0;
}
