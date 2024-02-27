#ifndef __ECHO_H
#define __ECHO_H

#include "process_pool.h"

class Echo
{
private:
    static const int BUFFER_SIZE = 1024;
    static int epollfd;
    int sockfd;
    sockaddr_in client_addr;
    char buff[1024] = {0};
public:
    Echo() {}
    ~Echo() {}

    void init(int _epollfd, int _sockfd, const sockaddr_in& address) {
        epollfd = _epollfd;
        sockfd = _sockfd;
        client_addr = address;
    }

    void process() {
        while(1) {
            memset(buff, 0, sizeof(buff));
            int ret = recv(sockfd, buff, sizeof(buff), 0);
            if(ret < 0) {
                if(errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
            } else if (ret == 0) {
                removefd(epollfd, sockfd);
                break;
            } else {
                send(sockfd, buff, sizeof(buff), 0);
            }
        }
        return;
    }
};

int Echo::epollfd = -1;

#endif