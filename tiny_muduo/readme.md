# 文件说明
epoller.h：定义了Epoller类，基于epoll实现事件轮询。
channel.h：定义了Channel类，用于管理I/O事件的通道。
eventloop.h：定义了EventLoop类，管理事件循环。
acceptor.h：定义了Acceptor类，用于接受新连接。
tcpserver.h：定义了TCPServer类，封装TCP服务器逻辑。
tcpconnectionptr.h：定义了TCPConnectionPtr类，表示TCP连接的智能指针。
echo.h：定义了EchoServer类，实现了一个简单的回显服务器。
callback.h：定义了回调函数类型。
address.h：定义了Address类，表示IP地址和端口号。

# 程序运行逻辑

主程序通过main.cc启动Echo服务器。
main.cc创建一个EventLoop实例，作为整个事件循环的管理者。
创建一个EchoServer实例，传入EventLoop实例和要监听的地址。
EchoServer内部创建了一个TCPServer实例，负责监听连接。
当有新连接到来时，TCPServer会自动调用注册的连接建立回调函数，并在其中创建一个TCPConnectionPtr实例表示新连接。
TCPConnectionPtr内部创建了一个Channel实例，用于管理连接的事件。
当有消息到来时，Channel会自动调用注册的消息到来回调函数，并在其中处理消息，然后将其原样发送回客户端。