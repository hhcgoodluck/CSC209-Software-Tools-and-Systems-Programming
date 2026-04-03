// Question Below From CSC209 Final Exam 2023 Winter

// Question8 [8 MARKS]
// The main function below creates a pipe and uses fork to create a single child process.
// The pipe will be used to communicate from the child to the parent.

/* Part (a) [6 marks]
   In the code below and on the next page if needed, complete the body of the if statement so that the parent
   process repeats the following steps NUM_READS times.
    1. Monitor the pipe and stdin until at least one is ready for reading.
    2. Read in up to BUFSIZE chars from the pipe and/or stdin (whichever are ready for reading) and
       then print the text that’s read to stdout. Remember to null-terminate the string before printing.
    • You may assume that the child process does not close its pipe write end or exit before the parent exits.
    • You may assume that the pipe’s file descriptors are > STDIN_FILENO, the file descriptor for stdin.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <string.h>
#include <sys/wait.h>

#define BUFSIZE 4096
#define NUM_READS 10

int main() {
    int pipefd[2];
    pipe(pipefd);
    int r = fork();
    if (r == 0) { // Child process
        // (code omitted)...
        } else if (r > 0) {    // Parent process
            close(pipefd[1]);  // parent only reads from the pipe

            int n;
            char buf[BUFSIZE+1];

            // 系统调用 select NUM_READS 次数 重置 read_fds 集合
            for (int i=0; i< NUM_READS; i++) {
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(STDIN_FILENO, &read_fds);  // add the stdin from user
                FD_SET(pipefd[0], &read_fds);     // add the pipe read end pipefd[0]

                // Note that file descriptor of stdin is 0
                int fdnum = pipefd[0] + 1;

                if (select(fdnum, &read_fds, NULL, NULL, NULL) == -1) {
                    perror("select");
                    exit(1);
                }

                // pipe’s file descriptors are > STDIN_FILENO
                if (FD_ISSET(STDIN_FILENO, &read_fds)) {
                    if ((n = read(STDIN_FILENO, buf, BUFSIZE)) < 0) {
                        perror("read");
                        exit(1);
                    } else {
                        buf[n] = '\0';
                        printf("%s", buf);
                    }
                }

                // child process read end file descriptor
                if (FD_ISSET(pipefd[0], &read_fds)) {
                    if ((n = read(pipefd[0], buf, BUFSIZE)) < 0) {
                        perror("read");
                        exit(1);
                    } else if (n == 0) {
                        printf("pipe from child 1 is closed\n");
                    } else {
                        buf[n] = '\0';
                        printf("%s\n", buf);
                    }
                }
            }
        }
    return 0;
}


// Part (b) [1 mark]
// Describe what you would need to change in your code if we remove the assumption that
// the child process does not close its pipe write end or exit before the parent exits.

// 之前假设: (1) child 不会提前退出 (2) child 不会提前关闭 pipe 的写端
// 去掉假设: read(pipefd[0], buf, BUFSIZE) 可能返回 n == 0

// If the child may close its write end or exit early, then the parent must handle end-of-file(EOF) on the pipe.
// In particular, if read(pipefd[0], ...) returns 0, the parent should close pipefd[0] and
// stop including it in the fd_set for future calls to select,
// otherwise select may keep reporting that fd as readable and read will keep returning 0.



// Part (c) [1 mark]
// Describe what you would need to change in your code if we remove the assumption that
// the pipe’s file descriptors are > STDIN_FILENO.

// change the logic about int numfd




















