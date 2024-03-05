# Note

## 1 Echo Server

Epoll+Nonblocking，单线程Reactor模式echo server。

1. 创建监听socket，绑定监听的本地地址（ip+端口号）。
2. 调用`listen()`，开始监听socket上的事件。
3. 创建内核事件表，注册监听socket上的读就绪事件，并设为非阻塞。
4. 调用`epoll_wait()`等待监听socket上有事件可读，将读就绪事件复制到`events`数组。
5. 处理`events`中的事件，若事件在监听socket上，则调用`accept()`，接受连接请求，并创建一个连接socket，注册到内核事件表（epollfd）。
6. 若事件在连接socket上，则调用`recv()`读取socket上的数据。
7. 若读取到的数据大小>0，则调用`send()`发送回数据；若数据大小=0，则关闭该连接socket。

<img src="assets\一般reactor.drawio.png" alt="一般reactor.drawio" style="zoom:50%;" />

## 2 Echo Server + multiprocess

使用Process Pool + One loop per thread方式。

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


## Tiny Muduo 多线程

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