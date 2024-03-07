[toc]

# Note

## ToDoList

- [ ] buffer类
- [ ] http状态机
- [ ] 同步/异步日志
- [ ] 定时器

## 1 Simple Echo Server

Epoll+Nonblocking，单线程Reactor模式echo server。

1. 创建监听socket，绑定监听的本地地址（ip+端口号）。
2. 调用`listen()`，开始监听socket上的事件。
3. 创建内核事件表，注册监听socket上的读就绪事件，并设为非阻塞。
4. 调用`epoll_wait()`等待监听socket上有事件可读，将读就绪事件复制到`events`数组。
5. 处理`events`中的事件，若事件在监听socket上，则调用`accept()`，接受连接请求，并创建一个连接socket，注册到内核事件表（epollfd）。
6. 若事件在连接socket上，则调用`recv()`读取socket上的数据。
7. 若读取到的数据大小>0，则调用`send()`发送回数据；若数据大小=0，则关闭该连接socket。

<img src="assets\一般reactor.drawio.png" alt="一般reactor.drawio" style="zoom:50%;" />

> socket编程和建立TCP连接的过程
>
> **客户端 connect 成功返回是在第二次握手，服务端 accept 成功返回是在三次握手成功之后。**

![半连接队列与全连接队列](assets\3.jpg)

## 2 Echo Server + multiprocess

使用Process Pool + One loop per process方式。

- 主进程监听listenfd的事件，若检测到连接请求，则使用round robin算法选择一个子进程，使用pipe通知子进程处理新的client连接请求。

- 子进程监听pipefd上的事件，若检测到事件（即主进程的通知），调用accept()处理listenfd上的连接请求并获取connfd，将connfd注册到epollfd。

- 子进程监听connfd上的事件，若检测到事件（即client发来的数据），调用用户处理类的process()，进行处理。

- 使用tmplate的目的：泛化，可以使用不同的user类对client的请求进行处理（实现中的user为Echo），只需要分别定义init()和process()函数。

*不是很懂MAX_USER_PER_PROCESS

## 3 Tiny Muduo

### 文件说明

epoller.h：定义了Epoller类，基于epoll实现事件轮询。

channel.h：定义了Channel类，用于管理I/O事件的通道。

eventloop.h：定义了EventLoop类，管理事件循环。

acceptor.h：定义了Acceptor类，用于接受新连接。

tcpserver.h：定义了TCPServer类，封装TCP服务器逻辑。

tcpconnectionptr.h：定义了TCPConnectionPtr类，表示TCP连接的指针。

echo.h：定义了EchoServer类，实现了一个简单的回显服务器。

callback.h：定义了回调函数类型。

address.h：定义了Address类，表示IP地址和端口号。

### 程序运行逻辑

1. 主程序通过main.cc启动Echo服务器，创建一个EventLoop实例，作为整个事件循环的管理者，并创建一个EchoServer实例，传入EventLoop实例和要监听的地址。

2. EchoServer内部创建了一个TCPServer实例，负责监听连接。并为其绑定连接建立时的回调函数（打印信息）和消息到来时的回调函数（send回消息）。

3. TCPServer内部又创建了一个Acceptor实例，用于接受新的连接。创建Acceptor实例时，会初始化listenfd，并进行bind地址；**且创建一个Channel实例，用于管理listenfd的事件**。

4. 当listenfd上有读事件时，Channel会自动调用注册的读事件回调函数（`Acceptor::NewConnection()`），在该函数中调用accept接受连接请求，并自动调用连接建立回调函数（TCPServer初始化创建Acceptor实例时便绑定好的`TCPServer::NewConnection(int connfd)`），在该函数中，会创建一个TCPConnectionPtr实例表示新连接。

5. TCPConnectionPtr内部**创建了一个Channel实例，用于管理connfd的事件**。当connfd上有读事件时，Channel会自动调用注册的读事件回调函数（`TCPConnectionPtr::HandleMessage()`），在其中调用Recv接收数据，若接收到数据，就调用message_callback_（即一开始从EchoServer中就绑定的消息回调函数），调用send将消息原样发送回客户端。

6. 上述各模块初始化完成、各回调函数绑定完成后，main.cc中调用`loop.Loop()`开始启动epoller轮询。将epoll_wait中获取到的所有就绪事件cast为channel，并置于active_channels_中。接着对每个channel中的事件，调用HandleEvent（即根据是读or写事件分别调用注册好的回调函数）。


## 4 Tiny Muduo: 线程池

thread pool + One loop per thread实现方式。

### 多线程实现逻辑

EventLoopThreadPool: 线程池，为TCPServer提供空闲的线程，处理新连接。

EventLoopThread: 封装线程类，做Pool和Thread之间的信息传递。

Thread: 最基本的线程类。

Mutex/Condition/Latch: 互斥锁、条件变量、屏障。

### 八线程循环使用

```
ThreadNums: 8
Start in Echo
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026722137856 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 26
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026713745152 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 27
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026705352448 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 28
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026696959744 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 29
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026688567040 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 30
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026680174336 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 31
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026335721216 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 32
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026327328512 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 33
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026722137856 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 34
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026713745152 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 35
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026705352448 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 36
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026696959744 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 37
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140026688567040 Manage
echo_server has a new connection 
Epoller Poll start
Epoller Poll end
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 38
Epoller Poll start
```

### 构造函数调用顺序

```
// 最开始构造EchoServer时，就把线程池初始化
[Cstr]: Epoller
[Cstr]: EventLoop
[Cstr]: EventLoopThreadPool
[Cstr]: Acceptor
[Cstr]: TCPServer
[Cstr]: EchoServer
// 初始化八个线程，进行epoll_wait
ThreadNums: 8
Start in Echo
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
[Cstr]: Thread
[Cstr]: EventLoopThread
[Cstr]: Epoller
[Cstr]: EventLoop
Epoller Poll start
Epoller Poll start
// 初始化完成，等待连接到来
Epoller Poll end
// listenfd收到连接请求，accept
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 5
TCPServer NewConnection Arrive Tid:140436439467776 Manage
[Cstr]: TCPConnectionPtr
echo_server has a new connection 
Epoller Poll start
// 等待client发送数据
Epoller Poll end
// connfd收到数据，进行recv
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 26
echo_server get message 
Epoller Poll start
// 继续等待
Epoller Poll end
// client断开连接
EventLoop::Loop eventnum 1
EventLoop::Loop connfd 26
Epoller Poll start
```

## Tiny Muduo: Buffer类

### Buffer的目的

non-blocking IO 的核心思想是避免阻塞在read()或write()或其他IO系统调用上，这样可以最大限度地复用thread-of-control，让一个线程能服务于多个socket连接。IO线程只能阻塞在IO multiplexing函数上，如select/poll/epoll_wait。这样一来，应用层的缓冲是必需的，每个TCP socket都要有stateful的input buffer和output buffer。

### muduo中的几个重要方法

https://blog.csdn.net/qq_42500831/article/details/124490830

**Buffer::Retrieve**

retrieve就是从Buffer读取数据。

**Buffer::makeSpace**

如果需要写入缓冲区数据的长度大于Buffer对象底层vector空闲的长度，进行扩容。(vector自动增长)

**Buffer::Append**

不管是从fd上读数据写到缓冲区inputBuffer_，还是发数据要写入outputBuffer_，我们都要往writeable区间内添加数据。

**Buffer::ReadFd**

给readv两个缓冲区，第一个就是Buffer对象的空间（一般是堆空间），第二个是65536字节的栈空间。readv会先写入第一个缓冲区，没写完再写入第二个缓冲区。如果读取了65536字节数据，fd上的数据还是没有读完，那就等Poller下一次上报（工作在LT模式），继续读取，数据不会丢失。

## Tiny Muduo: Http类

### Request Headers的一个示例

```
GET / HTTP/1.1
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
Accept-Encoding: gzip, deflate, br, zstd
Accept-Language: zh-CN,zh;q=0.9
Connection: keep-alive
Cookie: PSTM=1701226519; BIDUPSID=3F1D58F5B1AA8730667F78787E9423E2; BAIDUID=A7CBA5324C03CE0F889BAB2862377A1E:FG=1; BDUSS=jV3SzRYTDlpazdZTHpwNWVUazMxbGljY2dEdVFEMnhWYURXSnR6VEoydG1rLUZsRVFBQUFBJCQAAAAAAAAAAAEAAACEvG4i1cW37tHUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGYGumVmBrpla; BDUSS_BFESS=jV3SzRYTDlpazdZTHpwNWVUazMxbGljY2dEdVFEMnhWYURXSnR6VEoydG1rLUZsRVFBQUFBJCQAAAAAAAAAAAEAAACEvG4i1cW37tHUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGYGumVmBrpla; H_WISE_SIDS=40123_40155_40162_40201_39661_40207_40215_40223; BD_UPN=12314753; BDORZ=B490B5EBF6F3CD402E515D22BCDA1598; H_WISE_SIDS_BFESS=40123_40155_40162_40201_39661_40207_40215_40223; H_PS_PSSID=39661_40207_40215_40223_40294_40291_40288_40285_40317_40079_40364_40352_40366_40378; BAIDUID_BFESS=A7CBA5324C03CE0F889BAB2862377A1E:FG=1; BDRCVFR[feWj1Vr5u3D]=I67x6TjHwwYf0; BD_CK_SAM=1; PSINO=7; H_PS_645EC=d883dSdYnucUyC1laJn%2FZykaqzLzYrT9UX7O5KzzLX%2BX4dYM%2BbEH5TvpL3HuiOYnfl75; delPer=0; BDSVRTM=203; BA_HECTOR=058l8ka4ah0g0g8h0g8hag2gqg4s721iuifg91s; ZFY=7TnlXIu3rBzbMNyQWUWeE366bLHfJjDoTf1a2ce6X6E:C
Host: www.baidu.com
Referer: https://www.baidu.com/s?wd=http%E8%AF%B7%E6%B1%82%E6%A0%BC%E5%BC%8F&rsv_spt=1&rsv_iqid=0xf9e6b2560002d51c&issp=1&f=8&rsv_bp=1&rsv_idx=2&ie=utf-8&tn=baiduhome_pg&rsv_dl=tb&rsv_enter=1&rsv_sug3=18&rsv_sug2=0&rsv_btype=i&inputT=7409&rsv_sug4=7410
Sec-Fetch-Dest: document
Sec-Fetch-Mode: navigate
Sec-Fetch-Site: same-origin
Sec-Fetch-User: ?1
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36
sec-ch-ua: "Chromium";v="122", "Not(A:Brand";v="24", "Google Chrome";v="122"
sec-ch-ua-mobile: ?0
sec-ch-ua-platform: "Windows"
```

### 模块说明

**httpserver**

与echoserver类似，设置了连接、消息回调函数，以及对请求进行处理，发送response给客户。

**httpcontent**

每个TCPConnectionPtr内置一个content_，对每个连接逐行分析请求行、请求头、请求体。

**httprequest**

解析http请求的内容，获取method、http version等信息，更新状态机。

**httpresponse**

设置response的状态码、message、body等信息，并组成一整条栈上的字符串，append到buffer中，用于发送。