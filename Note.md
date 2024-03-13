[toc]

# Note

## ToDoList

- [x] buffer 类
- [x] http 状态机
- [ ] 同步/异步日志
- [ ] 定时器

## 1 Simple Echo Server

Epoll+Nonblocking，单线程 Reactor 模式 echo server。

1. 创建监听 socket，绑定监听的本地地址（ip+端口号）。
2. 调用`listen()`，开始监听 socket 上的事件。
3. 创建内核事件表，注册监听 socket 上的读就绪事件，并设为非阻塞。
4. 调用`epoll_wait()`等待监听 socket 上有事件可读，将读就绪事件复制到`events`数组。
5. 处理`events`中的事件，若事件在监听 socket 上，则调用`accept()`，接受连接请求，并创建一个连接 socket，注册到内核事件表（epollfd）。
6. 若事件在连接 socket 上，则调用`recv()`读取 socket 上的数据。
7. 若读取到的数据大小>0，则调用`send()`发送回数据；若数据大小=0，则关闭该连接 socket。

<img src="assets\一般reactor.drawio.png" alt="一般reactor.drawio" style="zoom:50%;" />

> socket 编程和建立 TCP 连接的过程
>
> **客户端 connect 成功返回是在第二次握手，服务端 accept 成功返回是在三次握手成功之后。**

![半连接队列与全连接队列](assets\3.jpg)

## 2 Echo Server + multiprocess

使用 Process Pool + One loop per process 方式。

- 主进程监听 listenfd 的事件，若检测到连接请求，则使用 round robin 算法选择一个子进程，使用 pipe 通知子进程处理新的 client 连接请求。

- 子进程监听 pipefd 上的事件，若检测到事件（即主进程的通知），调用 accept()处理 listenfd 上的连接请求并获取 connfd，将 connfd 注册到 epollfd。

- 子进程监听 connfd 上的事件，若检测到事件（即 client 发来的数据），调用用户处理类的 process()，进行处理。

- 使用 tmplate 的目的：泛化，可以使用不同的 user 类对 client 的请求进行处理（实现中的 user 为 Echo），只需要分别定义 init()和 process()函数。

\*不是很懂 MAX_USER_PER_PROCESS

## 3 Tiny Muduo

### 文件说明

epoller.h：定义了 Epoller 类，基于 epoll 实现事件轮询。

channel.h：定义了 Channel 类，用于管理 I/O 事件的通道。

eventloop.h：定义了 EventLoop 类，管理事件循环。

acceptor.h：定义了 Acceptor 类，用于接受新连接。

tcpserver.h：定义了 TCPServer 类，封装 TCP 服务器逻辑。

tcpconnectionptr.h：定义了 TCPConnectionPtr 类，表示 TCP 连接的指针。

echo.h：定义了 EchoServer 类，实现了一个简单的回显服务器。

callback.h：定义了回调函数类型。

address.h：定义了 Address 类，表示 IP 地址和端口号。

### 程序运行逻辑

1. 主程序通过 main.cc 启动 Echo 服务器，创建一个 EventLoop 实例，作为整个事件循环的管理者，并创建一个 EchoServer 实例，传入 EventLoop 实例和要监听的地址。

2. EchoServer 内部创建了一个 TCPServer 实例，负责监听连接。并为其绑定连接建立时的回调函数（打印信息）和消息到来时的回调函数（send 回消息）。

3. TCPServer 内部又创建了一个 Acceptor 实例，用于接受新的连接。创建 Acceptor 实例时，会初始化 listenfd，并进行 bind 地址；**且创建一个 Channel 实例，用于管理 listenfd 的事件**。

4. 当 listenfd 上有读事件时，Channel 会自动调用注册的读事件回调函数（`Acceptor::NewConnection()`），在该函数中调用 accept 接受连接请求，并自动调用连接建立回调函数（TCPServer 初始化创建 Acceptor 实例时便绑定好的`TCPServer::NewConnection(int connfd)`），在该函数中，会创建一个 TCPConnectionPtr 实例表示新连接。

5. TCPConnectionPtr 内部**创建了一个 Channel 实例，用于管理 connfd 的事件**。当 connfd 上有读事件时，Channel 会自动调用注册的读事件回调函数（`TCPConnectionPtr::HandleMessage()`），在其中调用 Recv 接收数据，若接收到数据，就调用 message*callback*（即一开始从 EchoServer 中就绑定的消息回调函数），调用 send 将消息原样发送回客户端。

6. 上述各模块初始化完成、各回调函数绑定完成后，main.cc 中调用`loop.Loop()`开始启动 epoller 轮询。将 epoll*wait 中获取到的所有就绪事件 cast 为 channel，并置于 active_channels*中。接着对每个 channel 中的事件，调用 HandleEvent（即根据是读 or 写事件分别调用注册好的回调函数）。

## 4 Tiny Muduo: 线程池

thread pool + One loop per thread 实现方式。

### 多线程实现逻辑

EventLoopThreadPool: 线程池，为 TCPServer 提供空闲的线程，处理新连接。

EventLoopThread: 封装线程类，做 Pool 和 Thread 之间的信息传递。

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

## 5 Tiny Muduo: Buffer 类

### Buffer 的目的

non-blocking IO 的核心思想是避免阻塞在 read()或 write()或其他 IO 系统调用上，这样可以最大限度地复用 thread-of-control，让一个线程能服务于多个 socket 连接。IO 线程只能阻塞在 IO multiplexing 函数上，如 select/poll/epoll_wait。这样一来，应用层的缓冲是必需的，每个 TCP socket 都要有 stateful 的 input buffer 和 output buffer。

### muduo 中的几个重要方法

https://blog.csdn.net/qq_42500831/article/details/124490830

**Buffer::Retrieve**

retrieve 就是从 Buffer 读取数据。

**Buffer::makeSpace**

如果需要写入缓冲区数据的长度大于 Buffer 对象底层 vector 空闲的长度，进行扩容。(vector 自动增长)

**Buffer::Append**

不管是从 fd 上读数据写到缓冲区 inputBuffer*，还是发数据要写入 outputBuffer*，我们都要往 writeable 区间内添加数据。

**Buffer::ReadFd**

给 readv 两个缓冲区，第一个就是 Buffer 对象的空间（一般是堆空间），第二个是 65536 字节的栈空间。readv 会先写入第一个缓冲区，没写完再写入第二个缓冲区。如果读取了 65536 字节数据，fd 上的数据还是没有读完，那就等 Poller 下一次上报（工作在 LT 模式），继续读取，数据不会丢失。

## 6 Tiny Muduo: Http 类

### Request Headers 的一个示例

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

与 echoserver 类似，设置了连接、消息回调函数，以及对请求进行处理，发送 response 给客户。

**httpcontent**

每个 TCPConnectionPtr 内置一个 content\_，对每个连接逐行分析请求行、请求头、请求体。

**httprequest**

解析 http 请求的内容，获取 method、http version 等信息，更新状态机。

**httpresponse**

设置 response 的状态码、message、body 等信息，并组成一整条栈上的字符串，append 到 buffer 中，用于发送。

## 7 Tiny Muduo 优化

- listen 的 backlog 改为 SOMAXCONN。

- 使用 shared_ptr 包装 TcpConnection。

- 使用 unique_ptr 包装 Channel & Epoller & Acceptor & EventLoopThreadPool。

- 使用右值引用+移动语义传递回调函数。

- 各个相关的类的析构函数中 close 不需要的文件描述符。

- 重用文件描述符，而不是每次来了请求就新建。

  - 如果状态是 kNew 或者 kDeleted，说明该文件描述符是新添加的或者已经被删除的。对于新添加的文件描述符，需要将其加入到 channels* 中，并且执行 EPOLL_CTL_ADD 操作来添加到 epoll 实例中，并更新该文件描述符的状态为 kAdded；对于已经被删除的文件描述符，也需要确保其在 channels* 中存在，并执行 EPOLL_CTL_ADD 操作来重新添加到 epoll 实例中，并更新状态为 kAdded。

  - 如果状态不是 kNew 或者 kDeleted，说明该文件描述符已经存在并且未被删除。如果事件为 0，则执行 EPOLL_CTL_DEL 操作删除该文件描述符的监视，并更新其状态为 kDeleted；否则，执行 EPOLL_CTL_MOD 操作修改该文件描述符的监视。

- 设置继承 Noncopyable 类，防止拷贝。

- 对线程相关的一些类改为显式构造。

### 并发测试

首先需要修改 linux 允许打开文件描述符最大数量：

[WSL2 更改打开文件限制（ulimit -n）的方法 - 知乎](https://zhuanlan.zhihu.com/p/500404323?utm_id=0)

WSL Ubuntu 环境下开 8 个线程的测试：

```
➜  webbench git:(master) ✗ ./webbench -c 1000 -t 30 http://127.0.0.1:2022/hello
  Webbench - Simple Web Benchmark 1.5
  Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

  Benchmarking: GET http://127.0.0.1:2022/hello
  1000 clients, running 30 sec.

  Speed=902626 pages/min, 752188 bytes/sec.
  Requests: 451313 susceed, 0 failed.
```

使用 lsof 查看系统已打开的文件：

```
LinuxWebServerDemo git:(master) ✗ lsof -p 7112 | grep sock
web_serve 7112 rthete   24u     sock      0,7      0t0 10668963 protocol: TCP
web_serve 7112 rthete   26u     sock      0,7      0t0 10668965 protocol: TCP
web_serve 7112 rthete   27u     sock      0,7      0t0 10668966 protocol: TCP
web_serve 7112 rthete   28u     sock      0,7      0t0 10668964 protocol: TCP
web_serve 7112 rthete   31u     sock      0,7      0t0  9074515 protocol: TCP
web_serve 7112 rthete   32u     sock      0,7      0t0  4558128 protocol: TCP
web_serve 7112 rthete   34u     sock      0,7      0t0  4558169 protocol: TCP
web_serve 7112 rthete   35u     sock      0,7      0t0  3631205 protocol: TCP
web_serve 7112 rthete   36u     sock      0,7      0t0  3702444 protocol: TCP
web_serve 7112 rthete   37u     sock      0,7      0t0  3664578 protocol: TCP
web_serve 7112 rthete   38u     sock      0,7      0t0  3664583 protocol: TCP
web_serve 7112 rthete   39u     sock      0,7      0t0  3702445 protocol: TCP
web_serve 7112 rthete   40u     sock      0,7      0t0  3739000 protocol: TCP
web_serve 7112 rthete   41u     sock      0,7      0t0  3739010 protocol: TCP
web_serve 7112 rthete   42u     sock      0,7      0t0  3761534 protocol: TCP
web_serve 7112 rthete   43u     sock      0,7      0t0  3761536 protocol: TCP
web_serve 7112 rthete   44u     sock      0,7      0t0  3880383 protocol: TCP
web_serve 7112 rthete   45u     sock      0,7      0t0  4055372 protocol: TCP
web_serve 7112 rthete   46u     sock      0,7      0t0  3880413 protocol: TCP
web_serve 7112 rthete   47u     sock      0,7      0t0  3880417 protocol: TCP
web_serve 7112 rthete   48u     sock      0,7      0t0  4189165 protocol: TCP
web_serve 7112 rthete   49u     sock      0,7      0t0  4016787 protocol: TCP
web_serve 7112 rthete   50u     sock      0,7      0t0  4055376 protocol: TCP
web_serve 7112 rthete   51u     sock      0,7      0t0  4232931 protocol: TCP
web_serve 7112 rthete   52u     sock      0,7      0t0  4055381 protocol: TCP
web_serve 7112 rthete   54u     sock      0,7      0t0  4411974 protocol: TCP
web_serve 7112 rthete   56u     sock      0,7      0t0  4538326 protocol: TCP
web_serve 7112 rthete   57u     sock      0,7      0t0  4538332 protocol: TCP
web_serve 7112 rthete   58u     sock      0,7      0t0  4774529 protocol: TCP
web_serve 7112 rthete   59u     sock      0,7      0t0  4629126 protocol: TCP
web_serve 7112 rthete   60u     sock      0,7      0t0  4590155 protocol: TCP
web_serve 7112 rthete   61u     sock      0,7      0t0  4981298 protocol: TCP
web_serve 7112 rthete   62u     sock      0,7      0t0  5008971 protocol: TCP
web_serve 7112 rthete   63u     sock      0,7      0t0  4981471 protocol: TCP
web_serve 7112 rthete   64u     sock      0,7      0t0  5009009 protocol: TCP
web_serve 7112 rthete   65u     sock      0,7      0t0  5058877 protocol: TCP
web_serve 7112 rthete   66u     sock      0,7      0t0  5480092 protocol: TCP
web_serve 7112 rthete   67u     sock      0,7      0t0  5480093 protocol: TCP
web_serve 7112 rthete   68u     sock      0,7      0t0  5305606 protocol: TCP
web_serve 7112 rthete   69u     sock      0,7      0t0  5480153 protocol: TCP
web_serve 7112 rthete   71u     sock      0,7      0t0  9402034 protocol: TCP
web_serve 7112 rthete   72u     sock      0,7      0t0  9448541 protocol: TCP
web_serve 7112 rthete   73u     sock      0,7      0t0  9539404 protocol: TCP
web_serve 7112 rthete   74u     sock      0,7      0t0  9539412 protocol: TCP
web_serve 7112 rthete   75u     sock      0,7      0t0  9485748 protocol: TCP
web_serve 7112 rthete   76u     sock      0,7      0t0  9617216 protocol: TCP
web_serve 7112 rthete   77u     sock      0,7      0t0  9617301 protocol: TCP
web_serve 7112 rthete   78u     sock      0,7      0t0  9696606 protocol: TCP
web_serve 7112 rthete   79u     sock      0,7      0t0  9696943 protocol: TCP
web_serve 7112 rthete   80u     sock      0,7      0t0  9696945 protocol: TCP
web_serve 7112 rthete   81u     sock      0,7      0t0  9696946 protocol: TCP
web_serve 7112 rthete   82u     sock      0,7      0t0  9766352 protocol: TCP
web_serve 7112 rthete   83u     sock      0,7      0t0  9803630 protocol: TCP
web_serve 7112 rthete   84u     sock      0,7      0t0  9791699 protocol: TCP
```

### 使用 muduo 测试

[C++ muduo 网络库知识分享 01 - Linux 平台下 muduo 网络库源码编译安装](https://blog.csdn.net/QIANGWEIYUAN/article/details/89023980)

运行了 httpserver_test，使用 Webbench 测试并发：

```
➜  webbench git:(master) ✗ ./webbench -c 1000 -t 30 http://0.0.0.0:8000/hello
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Benchmarking: GET http://0.0.0.0:8000/hello
1000 clients, running 30 sec.

Speed=2340146 pages/min, 3627229 bytes/sec.
Requests: 1170073 susceed, 0 failed.
```

## 附 1-关于 muduo 的总结

[Muduo 启动 && 收到请求时的流程梳理（文字版）](https://zhuanlan.zhihu.com/p/606436239)

### 启动时发生的事

1. 创建主 loop，执行初始化操作：

   - bool looping\_ （当前是否处于循环状态）初始化为 false
   - bool quit\_ （是否退出循环标识）初始化为 false
   - bool CallingPendingFunctors\_ （当前 loop 是否有要执行的回调操作）初始化为 false
   - threadId\_ （当前 loop 所在线程 id） 初始化为当前线程 id
   - EPollPoller poller\_ （EPollPoller 实例）初始化：
     - EventLoop ownerLoop\_：用传过来的 loop 对该成员变量进行初始化
     - epollfd：创建一个 epoll 文件描述符
     - std::vector<epoll_event> events\_：初始化一个存放 epoll_event 的 vector
   - wakeupFd\_：创建一个用于唤醒的 fd，当 loop 阻塞住时，通过调用它进行写入来唤醒 loop 执行进一步操作
   - Channel wakeupChannel\_：wakeupfd 对应的 channel
   - wakeupchannel 设置 read 回调：EventLoop::handleRead
   - 从 wakeupfd 的文件中读一个数来唤醒
   - 设置 wakeupchannel 对应的 fd 为可读状态

2. 初始化 EchoServer:
   - TcpServer server\_：初始化 tcpserver
     - EventLoop loop\_：主 loop
     - string ipPort\_：用户设置的 port
     - string name\_：用户设置的 服务名
     - std::unique_ptr<Acceptor> acceptor\_：用 loop, address 初始化一个 acceptor
3. 初始化 Acceptor 的操作：

   - EventLoop loop\_：用主 loop 初始化
   - Socket acceptSocket\_：调用系统 socket 生成对应的 sockfd，并初始化自己定义的 Socket 类（用 sockfd 初始化）

     - socket 设置地址、端口复用，并绑定地址端口
     - 绑定回调：如果有 新用户链接/read 操作，就执行 Acceptor::handleRead 操作
     - 如果 accept 有效，获取 客户端 connfd，执行上层给的新建立链接时的回调函数 TcpServer::newConnection
     - 轮询选择一个 subloop 来管理新链接 connfd 的 channel
     - 初始化一个 TcpConnectionPtr conn (new TcpConnection)：
       - 初始化 EventLoop loop\_ (子 loop）
       - 初始化 connection 的 name\_
       - 初始化 connection 的 state\_ 为 kConnecting
       - 初始化 connection 是否为读状态 reading\_ 为 true
       - 初始化建立一个 socket 是复用的？
       - 初始化一个 channel （子 loop + 传过来的 sockfd）
       - 初始化 highWaterMark\_ 64 M
       - 设置 channel 的 读、写、关闭、异常 回调函数

     > Channel 的各种回调
     >
     > - 读回调：TcpConnection::handleRead，读是相对服务器而言的，当对端客户端有数据到达，服务器端检测到 EPOLLIN 就会触发该 fd 上的回调。handleRead 读走对端发来的数据。实际上是调用 `Buffer inputBuffer_.readFd` 来取走这个 buffer 缓冲区的数据。同时触发用户设置的 onMessage 回调函数
     >
     > - 写回调：TcpConnection::handleWrite，和上面的类似，调用 inputBuffer.writefd 像缓冲区写数据，同时调用用户设置的 writeCompleteCallback 回调函数
     >
     > - 关闭回调：TcpConnection::handleClose， 设置相应的 channel 状态 和 connection 状态。并执行相应的 关闭回调 connectionCallback\_
     >
     > - 异常回调：TcpConnection::handleError

     - 设置 TcpConnectionPtr 的链接、收到消息、写完、关闭链接 对应的回调函数

     - 在 上面被选择的 subloop 下，运行 TcpConnection::connectEstablished 回调：设置 channel 链接状态

   - Channel acceptChannel：用 sockfd 和 loop 初始化一个 acceptChannel
   - bool listenning\_：是否在监听，设置为 false
   - 用主 loop 创建一个 EventLoopThreadPool threadPool\_ ：

     - 设置一个 baseLoop
     - 设置 pool 的名称
     - 设置是否启动状态 bool started(false)
     - 设置线程数 ，初始化 0
     - 设置下一个 loop 轮询的下标 的 next\_ 为 0

   - 初始化 connectionCallback
   - 初始化 messageCallback
   - 初始化 下一个链接的 id nextConnId
   - 设置 started\_ 为 0
   - 把主 loop 赋给 EchoServer loop\_
   - 设置用户定义的 server.setConnectionCallback 到前面的 connectionCallback
   - 设置用户定义的 server.setMessageCallback 到前面的 messageCallback\_
   - 设置 subloop 数量 server\_.setThreadNum

4. 启动服务：EchoServer server.start() --> TCPServer.start()

- EventLopThreadPool -> start(threadInitCallback\_):
  - 根据设置的 subloop 数量，循环生产 EventLoopThread 放到 std::vector<std::uniqueptr<EventLoopThread>> threads\_ 中。std::vector<EventLoop _> loops 中存放 EventLoopThread::startLoop 返回的 子 loop
- loop\_->runInLoop(std::bind(&Acceptor::listen, acceptor\_.get()))：
  - 如果在当前 loop 中执行回调，则直接运行 Acceptor::listen
  - 否则把回调函数先存起来，用 wakeup 函数唤醒 loop 所在线程去执行相应的回调

5. loop 开启事件循环：EventLoop::loop

- 调用 EPollPoller::poll 获取发生事件的 channel 列表
  - 调用 epoll_wait 获取发生事件的 epoll_enent
  - 调用 fillActiveChannels(numEvents, activeChannels) 将 活动的 events\_ 转化为 channel 的 ChannelList \_activeChannels 并返回
- 对所有发生事件的 channel，执行 Channel:: handleEvent --> Channel:: handleEventWithGuard
  - 针对发生的不同事件类型，执行相应的 callback

### 当有新请求连接时发生的事

1. accept 接收连接， 主 loop 的 epoller-> poll 监听到连接事件 epoll_event
2. 将该连接 epoll_event 包装成 channel
3. TcpServer::newConnection 通过轮询将 TcpConnection 对象分配给 subloop 处理
