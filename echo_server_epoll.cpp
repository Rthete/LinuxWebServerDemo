#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define MAX_EVENTS_NUMBER 5

/**
 * 将文件描述符设置为非阻塞
*/
int set_non_blocking(int fd) {
    // 获取旧状态标志
    int old_state = fcntl(fd, F_GETFL);
    // 设置非阻塞标志
    int new_state = old_state | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_state);
    // 返回旧状态标志，以便日后恢复该状态标志
    return old_state;
}

void addfd(int epollfd, int fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    // 向事件表中注册fd上的事件
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    set_non_blocking(fd);
}

int main(int argc, char* argv[]) {
    if(argc <= 2) {
        printf( "Usage: %s ip_address portname\n", argv[0] );
		return 0;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 1);

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int ret = 0;
    ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENTS_NUMBER];
    // 创建内核事件表
    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    while(1) {
        /** epoll_wait函数
         *      在一段超时时间内等待epollfd上的事件。
         *      如果检测到事件，就将所有就绪的事件从内核事件表中复制到events数组中。
         *      只输出`就绪`的事件。
        */
        int number = epoll_wait(epollfd, events, MAX_EVENTS_NUMBER, -1);
        if(number < 0) {
            printf("epoll_wait failed\n");
            return -1;
        }
        for(int i = 0; i< number; i++) {
            printf("i: %d\n", i);
            const auto& event = events[i];
            const auto eventfd = event.data.fd;

            if(eventfd == listenfd) {
                printf("event: listenfd, fd = %d\n", eventfd);
                struct sockaddr_in client;
                socklen_t client_addrlength = sizeof(client);
                int sockfd = accept(listenfd, (struct sockaddr*)(&address), &client_addrlength);
                addfd(epollfd, sockfd);
            } else if (event.events & EPOLLIN) {
                printf("event: EPOLLIN, fd = %d\n", eventfd);
                char buff[1024] = {0};
                while(1) {
                    memset(buff, '\0', sizeof(buff));
                    int recv_size = recv(eventfd, buff, sizeof(buff), 0);
                    if(recv_size < 0) {
                        // EAGAIN: 再来一次
                        // EWOULDBLOCK: 期望阻塞
                        if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            break;
                        }
                        printf("sockfd %d, recv msg failed\n", eventfd);
                        close(eventfd);
                        break;
                    } else if (recv_size == 0) {
                        printf("recv_size == 0, close eventfd %d\n", eventfd);
                        close(eventfd);
                        break;
                    } else {
                        send(eventfd, buff, recv_size, 0);
                    }
                }
            } 
        }
    }
    close(listenfd);
    return 0;
}