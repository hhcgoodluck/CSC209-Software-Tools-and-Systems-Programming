// Question Below From CSC209 Final Exam 2017 Winter

// Question 9. [10 MARKS]
// Consider the following parts of a server that will manage multiple clients.
// Note that error checking has been removed for brevity and some parts of the program are missing.
// The questions below assume that the missing components are correctly implemented.

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAXNAME 256

// 客户端结构体节点 -> 客户端链表
struct client {
    int fd;       // 客户端的 socket 文件描述符
    char *name;   // 客户端的名字（字符串，动态分配）
    struct client *next;  // 指向下一个客户端（链表）
};

// 客户端链表操作 addclient 函数
// 将一个新客户端插入到链表头部，并返回新的链表头
static struct client *addclient(struct client *top, int fd, char *name) {
    // 在堆上申请一个 client 结构体 malloc 动态内存分配
    struct client *p = malloc(sizeof(struct client));
    // 初始化 client 结构体 p → [ fd=?, name=?, next=? ]
    p->fd = fd;
    p->name = malloc(strlen(name)+1);
    strncpy(p->name, name, strlen(name)+1);

    // 头指针插法 新节点指向原来的链表头
    p->next = top;
    // 更新头指针 让链表头变成新节点
    top = p;
    // 返回更新的头指针
    return top;
}

int main(int arge, char **argv) {
    fd_set readset;   // 系统函数 select 文件描述符读取集合
    int maxfd;        // 系统函数 select 当前最大的 fd 数值
    int n;            // 系统函数 select 返回值

    int listenfd;     // 监听 socket
    int clientfd;     // 新接收的客户端 fd

    socklen_t len;                 // 地址长度

    struct sockaddr_storage q;     // 用来接收客户端地址信息
    struct client *head = NULL;    // 客户端链表头，初始为空

    // Assume all variables are correctly declared and initialized
    // The appropriate calls have been made to set up the socket.

    FD_ZERO(&readset);
    FD_SET(listenfd, &readset);
    maxfd = listenfd;

    // infinite while loop
    while (1) {
        n = select(maxfd + 1, &readset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &readset)) {
            clientfd = accept(listenfd, (struct sockaddr *)&q, &len);
            char buffer[MAXNAME];
            read(clientfd, buffer, MAXNAME);
            printf ("connection from 70s\n", buffer);
            head = addclient(head, clientfd, buffer);
        }
    }
    // code to check each client that is ready to read follow

    return 0;
}
