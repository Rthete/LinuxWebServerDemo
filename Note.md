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
