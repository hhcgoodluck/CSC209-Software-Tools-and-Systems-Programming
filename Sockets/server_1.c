// Sockets Video 2: Socket Configuration

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>    /* Internet domain header */
#include <arpa/inet.h>     /* only needed on my mac */

// 基础的 TCP 服务器初始化流程
// 核心就是三步：socket -> bind -> listen (-> accept)

// 1. 创建 socket（相当于创建“电话”）
// 2. 绑定地址（给电话分配号码和位置）
// 3. 开始监听（等待别人打电话）

int main() {
    // create socket 创建通信端点(服务器)
    // 参数解释: AF_INET: (IPv4)   SOCK_STREAM: TCP（面向连接）  0: 默认协议（自动选 TCP）
    // 系统函数: socket
    int listen_soc = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_soc == -1) {
        perror("server: socket");
        exit(1);
    }

    // initialize server address 初始化服务器地址
    // 网络地址结构体: sockaddr_in
    struct sockaddr_in server;            // 专用于 IPv4 地址 结构体
    server.sin_family = AF_INET;          // 地址类型 = IPv4
    server.sin_port = htons(54321);       // 端口号设置 (htons) host to network short
    memset(&server.sin_zero, 0, 8);       // 填充位清零（结构体对齐用）
    server.sin_addr.s_addr = INADDR_ANY;  // 绑定到本机所有 IP

    // bind socket to an address 将网络端口绑定地址
    // 系统函数: bind
    if (bind(listen_soc, (struct sockaddr *) &server, sizeof(struct sockaddr_in)) == -1) {
        perror("server: bind");
        close(listen_soc);
        exit(1);
    }
    // Note: 强制类型转换 (struct sockaddr *)
    // Note: sockaddr_in 是具体类型  sockaddr 是通用接口类型
    // Note: 所有 socket API 都用 sockaddr

    // Set up a queue in the kernel to hold pending connections.
    // 开始监听 socket 进入监听状态 普通 socket 变成 server socket
    // 系统函数: listen
    if (listen(listen_soc, 5) < 0) {
        // listen failed
        perror("listen");
        exit(1);
    }
    // Note: (等待队列长度) backlog 同时最多允许多少个“正在排队连接”的客户端

    // 客户端 client 地址结构 为 accept() 准备
    struct sockaddr_in client_addr;
    unsigned int client_len = sizeof(struct sockaddr_in);
    client_addr.sin_family = AF_INET;

    // int return_value = accept(listen_soc, (struct sockaddr *)&client_addr, &client_len);
    return 0;
}
