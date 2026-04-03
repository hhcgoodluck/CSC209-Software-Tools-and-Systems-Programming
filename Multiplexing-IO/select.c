#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAXSIZE 4096
void handle_child1(int *fd);
void handle_child2(int *fd);

/* A program to illustrate the basic use of select.
 *
 * The parent forks two children with a pipe to read from each of them and then
 * reads first from child 1 followed by a read from child 2.
*/

int main() {
    char line[MAXSIZE];
    int pipe_child1[2], pipe_child2[2];

    // Before we fork, create a pipe for child 1
    if (pipe(pipe_child1) == -1) {
        perror("pipe");
    }

    int r = fork();
    if (r < 0) {
        perror("fork");
        exit(1);
    } else if (r == 0) {
        handle_child1(pipe_child1);
        exit(0);
    } else {
        // This is the parent. Fork another child,
        // but first close the write file descriptor to child 1
        close(pipe_child1[1]);
        // and make a pipe for the second child
        if (pipe(pipe_child2) == -1) {
            perror("pipe");
        }

        // Now fork the second child
        r = fork();
        if (r < 0) {
            perror("fork");
            exit(1);
        } else if (r == 0) {
            close(pipe_child1[0]);  // still open in parent and inherited
            handle_child2(pipe_child2);
            exit(0);
        } else {
            close(pipe_child2[1]);

            // This is now the parent with 2 children
            // each with a pipe from which the parent can read.

            // Update Version: continue to listen two processes message

            // 定义文件描述符集合(监视的文件描述符名单)
            fd_set read_fds; // 名称表示关心哪些 fd 是否可读
            // 初始化集合为空集 准备重新登记
            FD_ZERO(&read_fds);
            // 添加两个管道的读取端到监听集合中
            FD_SET(pipe_child1[0], &read_fds);
            FD_SET(pipe_child2[0], &read_fds);

            // 系统函数 select 参数 (int) numfd
            // 监视的文件描述符(int)范围  [0~numfd)
            // 待监视的文件描述符(int)的范围上限+1 (比较后较大的fd值+1)
            int numfd;
            if (pipe_child1[0] > pipe_child2[0]) {
                numfd = pipe_child1[0] + 1;
            } else {
                numfd = pipe_child2[0] + 1;
            }

            if (select(numfd, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select");
                exit(1);
            }

            // Read first from child 1
            if (FD_ISSET(pipe_child1[0], &read_fds)) {
                if ((r = read(pipe_child1[0], line, MAXSIZE)) < 0) {
                    perror("read");
                } else if (r == 0) {
                    printf("pipe from child 1 is closed\n");
                } else {
                    printf("Read %s from child 1\n", line);
                }
            }

            // Now read from child 2
            if (FD_ISSET(pipe_child2[0], &read_fds)) {
                if ((r = read(pipe_child2[0], line, MAXSIZE)) < 0) {
                    perror("read");
                } else if (r == 0) {
                    printf("pipe from child 2 is closed\n");
                } else {
                    printf("Read %s from child 2\n", line);
                }
            }
        }
        // could close all the Pipes but since program is ending we will just let
        // them be closed automatically
    }
    return 0;
}

// child1 process performance function
void handle_child1(int *fd) {
    close(fd[0]);  // we are only writing from child to parent
    printf("[%d] child\n", getpid());
    // Child will write to parent
    char message[10] = "HELLO DAD";
    write(fd[1], message, 10);
    close(fd[1]);
}

// child2 process performance function
void handle_child2(int *fd) {
    close(fd[0]);  // we are only writing from child to parent
    printf("[%d] child\n", getpid());
    // Child will write to parent
    char message[10] = "Hi mom";
    write(fd[1], message, 10);
    close(fd[1]);
}

// Note: 上边代码仅调用一次 select 系统函数
// Note: 程序行为等到存在or至少有一个 pipe ready，然后读这一次 ready 的 fd

// Update: 持续监听两个子进程的程序设计(调用多次 select 系统函数)
// while (1) {
//     FD_ZERO(&read_fds);
//     FD_SET(pipe_child1[0], &read_fds);
//     FD_SET(pipe_child2[0], &read_fds);
//
//     select(numfd, &read_fds, NULL, NULL, NULL);
//
//     if (FD_ISSET(pipe_child1[0], &read_fds)) {
//         read(...)
//     }
//     if (FD_ISSET(pipe_child2[0], &read_fds)) {
//         read(...)
//     }
// }

// Note: 因为 select 每次调用后都会改写 fd_set，下一轮必须重新 FD_ZERO 和 FD_SET
// Note: 重置构建 fd_set 集合, 每次 select 调用会破坏监视名单

// Note: FD_ISSET(fd, &read_fds) 检查这个 fd 在 select 返回后是否仍然在集合中(已经准备好)
// Note: select 会把传进去的 fd_set 从“监视列表”突变改成“就绪列表”
// Note: 调用 select 之前(想监视哪些 fd)；调用 select 之后(哪些 fd 已经 ready)

// Because select modifies the fd_set in place, removing file descriptors that are not ready.
// Therefore, before each call to select, we must reinitialize the fd_set
// using FD_ZERO and re-add all file descriptors of interest using FD_SET,
// otherwise some descriptors may no longer be monitored.
