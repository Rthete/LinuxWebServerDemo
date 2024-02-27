#ifndef __PROCESS_POOL_H_
#define __PROCESS_POOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h> // fork
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>

class Process
{
public:
    int pid;        // 进程pid
    int pipe[2];    // 通信管道，主进程pipe[0]，子进程pip[1]

    Process(): pid(-1), pipe{0, 0} {}
};

template<typename T>
class ProcessPool
{
private:
    static const int MAX_EVENTS_NUMBER = 5;
    static const int MAX_USER_PER_PROCESS = 10000;
    int idx;
    int listenfd;
    int epollfd;
    int max_process_num;
    Process* sub_process;
    static ProcessPool<T>* instance;

    // 私有的构造函数
    ProcessPool(int listenfd, int max_process_num = 8);
    ~ProcessPool() {
        std::cout << "ProcessPool destructor" << std::endl;
        delete[] sub_process;
    }
public:
    // 静态create函数，用于创建单例的ProcessPool对象
    static ProcessPool<T>* create(int listenfd, int _max_process_num) {
        if(instance == nullptr) {
            instance = new ProcessPool<T>(listenfd, _max_process_num);
            return instance;
        }
        return instance;
    }

    void run();
    void run_parent();
    void run_child();
    void setup_up_sig();
};

// 静态对象初始化
template<typename T>
ProcessPool<T>* ProcessPool<T>::instance = nullptr;

template<typename T>
ProcessPool<T>::ProcessPool(int listenfd, int _max_process_num):
                        idx(-1), listenfd(listenfd), epollfd(0),
                        max_process_num(_max_process_num), sub_process(nullptr)
{
    sub_process = new Process[max_process_num];
    for(int i = 0; i < max_process_num; i++) {
        socketpair(PF_UNIX, SOCK_STREAM, 0, sub_process[i].pipe);
        // fork函数：对主进程返回子进程的pid，对子进程返回0
        sub_process[i].pid = fork();

        if(sub_process[i].pid > 0) {
            // 在主进程上，关闭pipe[1]，只使用pipe[0]
            close(sub_process[i].pipe[1]);
            continue;
        } else {
            // 在子进程上，关闭pipe[0]，只使用pipe[1]
            close(sub_process[i].pipe[0]);
            idx = i;
            break;
        }
    }
}

static int set_non_blocking(int fd) {
    int old_state = fcntl(fd, F_GETFL);
    int new_state = old_state | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_state);

    return old_state;
}

static void addfd(int epollfd, int fd) {
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = fd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    set_non_blocking(fd);
}

static void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

template<typename T>
void ProcessPool<T>::run() {
    if(idx == -1) {
        run_parent();
    } else {
        run_child();
    }
}

template<typename T>
void ProcessPool<T>::setup_up_sig() {
    epollfd = epoll_create(5);
    assert(epollfd != -1);
}

template<typename T>
void ProcessPool<T>::run_parent() {
    epoll_event events[MAX_EVENTS_NUMBER];
    setup_up_sig();
    addfd(epollfd, listenfd);

    int pre_idx = 0;
    int has_new_cli = 1;
    int number = 0;
    while(1) {
        number = epoll_wait(epollfd, events, MAX_EVENTS_NUMBER, -1);
        for(int i = 0; i < number; i++) {
            int eventfd = events[i].data.fd;
            // 主进程，用于监听listenfd上的事件
            if(eventfd == listenfd) {
                // round-robin
                // 通过取模的方式，轮流将请求分给子进程
                int pos = pre_idx;
                do {
                    pos = (pos + 1) % max_process_num;
                } while (sub_process[pos].pid == -1);
                pre_idx = pos;
                // 通过管道通知第pos个子进程，处理请求
                send(sub_process[pos].pipe[0], (void*)&has_new_cli, sizeof(has_new_cli), 0);
                printf("parent process has sent msg to %d child\n", pos);
            }
        }
    }
}

template<typename T>
void ProcessPool<T>::run_child() {
    epoll_event events[MAX_EVENTS_NUMBER];
    setup_up_sig();

    int pipefd = sub_process[idx].pipe[1];
    addfd(epollfd, pipefd);
    T* users = new T[MAX_USER_PER_PROCESS];

    int number = 0;
    while(1) {
        number = epoll_wait(epollfd, events, MAX_EVENTS_NUMBER, -1);
        for(int i = 0; i < number; i++) {
            int eventfd = events[i].data.fd;
            // 若EPOLLIN事件发生在pipefd上:
            // pipefd上收到主进程通知，子进程开始处理连接请求
            if(eventfd == pipefd && (events[i].events & EPOLLIN)) {
                struct sockaddr_in client;
                socklen_t client_addrlength = sizeof(client);
                int connfd = accept(listenfd, (struct sockaddr*)(&client), &client_addrlength);
                addfd(epollfd, connfd);
                users[connfd].init(epollfd, connfd, client);
                printf("child %d is addfd-ing\n", idx);
                continue;
            // 若EPOLLIN事件发生在connfd上：
            // users对收到的数据进行处理
            } else if (events[i].events & EPOLLIN) {
                printf("child %d has recv msg\n", idx);
                users[eventfd].process();
            }
        }
    }
    delete[] users;
    users = nullptr;

    close(epollfd);
    close(pipefd);
}

#endif