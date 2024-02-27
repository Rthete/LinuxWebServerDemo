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