#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

/* Demonstrate how file redirection is implemented using dup2.  */

int main() {
    int result;

    result = fork();

    /* 子进程: The child process will call exec  */
    if (result == 0) {
        // int filefd = open("day.txt", O_RDWR | S_IRWXU | O_TRUNC);
		// 文本文件 day.txt 文件描述符 int 类型
        int filefd = open("day.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        if (filefd == -1) {
            perror("open");
        }
		// 重定向: stdout > file (复制 fileno(stdout) := filefd)
		// 原来 stdout 指向: 终端(屏幕); 现在 stdout 指向: filefd 对应文件
        if (dup2(filefd, fileno(stdout)) == -1) {
            perror("dup2");
        }
		// 原来的 filefd 不再使用可以关闭; 不关闭会导致 文件描述符泄漏
        close(filefd);

		// 系统调用 exec
		// 用一个新程序替换当前进程 grep L0101 student_list.txt
        execlp("grep", "grep", "L0101", "student_list.txt", NULL);
        perror("exec");
        exit(1);

    } else if (result > 0) {	// 父进程
        int status;
        printf("HERE\n");
        if (wait(&status) != -1) {
            if (WIFEXITED(status)) {
                fprintf(stderr, "Process exited with %d\n",
                        WEXITSTATUS(status));
            } else {
                fprintf(stderr, "Process teminated\n");
            }
        }

    } else {
        perror("fork");
        exit(1);
    }
    return 0;
}



// 系统调用 dup2: duplicate file descriptor (to a specified number)
// 函数原型: int dup2(int oldfd, int newfd);	dup = duplicate(复制); 2 = 第二种版本(可以指定目标 fd)
// 文件描述符复制or重新赋值 (newfd := oldfd)

// eg1: dup2(filefd, fileno(stdout));  重新定向输出 stdout > file
// old_fd: filefd	new_fd: 重定向后等价于 fileno(stdout)
// printf("hello\n"); 将不会输出打印到屏幕 而是写入文件

// eg2: dup2(filefd, fileno(stdin));	重新定向输入 stdin > file
// scanf(); 将会从文件读取 而不是从屏幕

// eg3: dup2(pipefd[1], fileno(stdout));	重新定向输出  stdout > pipe



// 系统调用 execlp: 在当前进程中执行一个新程序 并把当前进程彻底替换掉
// exec：执行新程序    l:list(参数逐个列出来)    p:path(在 PATH 环境变量中查找程序)
// 函数原型: int execlp(const char *file, const char *arg0, ..., NULL);











