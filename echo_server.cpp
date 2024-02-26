#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char* argv[]) {
    if (argc <= 2) {
        printf("Usage: %s ip_address portname \n", argv[0]);
        return 0;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    // 服务端监听端口
    /** domain参数: 底层协议族
     *      PF_INET: IPv4
     *      PF_INET6: IPv6
     *      PF_UNIX: UNIX本地域协议族
    */
    /** type参数: 服务类型
     *      SOCK_STREAM: 流服务
     *      SOCK_UGRAM: 数据报服务
    */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 1);

    /** sockaddr_in
     * sockaddr_in是一个结构体，用于表示Internet地址和端口号。它定义在头文件<netinet/in.h>中，通常与套接字（socket）API一起使用。
     * sockaddr_in结构体包含以下字段：
     *   sin_family：地址族，通常设置为AF_INET表示IPv4协议。
     *   sin_port：端口号，以网络字节序表示。
     *   sin_addr：IP地址，以网络字节序表示。
     *   sin_zero：填充字段，通常设置为0。
    */
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    /** 字节序转换
     *  htonl/htons/ntohl/ntohs
     *  长整型(long)通常转换IP地址
     *  短整型(short)通常转换端口号
    */
    address.sin_port = htons(port); // host to network short
    /** IP地址转换函数
     *      inet_addr: 点分十进制地址(ipv4)-->网络字节序二进制整数(ipv4)
     *      inet_aton: 点分十进制地址(ipv4)-->网络字节序二进制整数(ipv4)
     *      inet_ntoa: 网络字节序二进制整数(ipv4)-->点分十进制地址(ipv4)
     *      inet_pton: 点分十进制地址(ipv4)/十六进制ipv6地址-->网络字节序二进制整数
     *      inet_ntop: 网络字节序二进制整数-->点分十进制地址(ipv4)/十六进制ipv6地址
    */
    inet_pton(AF_INET, ip, &address.sin_addr);

    int ret = 0;
    /** 专用socket地址类型->强制转换->通用socket地址类型
     *  专用socket地址类型: 
     *      sockaddr_un(UNIX本地域协议族)
     *      sockaddr_in(TCP/IP协议族-IPv4)
     *      sockaddr_in6(TCP/IP协议族-IPv6)
     *  通用socket地址类型：
     *      sockaddr(14字节)
     *      sockaddr_storage(内存对齐)
    */
    ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
    assert(ret != -1);

    // 服务端开始监听
    ret = listen(listenfd, 5);
    assert(ret != -1);

    // 接受客户端连接请求
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int sockfd = accept(listenfd, (struct sockaddr*)(&client), &client_addrlength);

    // 服务端接收数据并存储到buff中
    char buff[1024] = {0};
    int recv_size = 0;
    recv_size = recv(sockfd, buff, sizeof(buff), 0);

    // 服务端将接收的数据原样发送回客户端
    int send_size = 0;
    send_size = send(sockfd, buff, recv_size, 0);

    close(sockfd);
    close(listenfd);

    return 0;
}