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

    /* The child process will call exec  */
    if (result == 0) {
        //int filefd = open("day.txt", O_RDWR | S_IRWXU | O_TRUNC);
        int filefd = open("day.txt", O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
        if (filefd == -1) {
            perror("open");
        }
        if (dup2(filefd, fileno(stdout)) == -1) {
            perror("dup2");
        }
        close(filefd);
        execlp("grep", "grep", "L0101", "student_list.txt", NULL);
        perror("exec");
        exit(1);

    } else if (result > 0) {
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

// sample input student_list.txt
// g5hopper  Hopper  Grace         L0101
// g4bartik  Bartik  Jean          L5101
// g5perlma  Perlman  Radia        L0101
// g5allenf  Allen  Frances        L0101
// g5liskov  Liskov  Barbara       L5101
// g4tardos  Tardos Eva            L0101
// g5goldwa  Goldwasser  Shafi     L5101
// g4klawem  Klawe  Maria          L5101
// g5wingje  Wing  Jeanette        L0101
// g4borgan  Borg  Anita           L0101
// g4worsel  Worsley  Beatrice     L5101
// g4mcnult  McNulty  Kay          L0101
// g5snyder  Snyder  Betty         L5101
// g5wescof  Wescoff  Marlyn       L0101
// g5lichte  Lichterman  Ruth      L5101
// g4jennin  Jennings  Betty       L0101
// g5bilasf  Bilas  Fran           L0101
// g5schnei  Schneider  Erna       L5101
// g4sammet  Sammet  Jean          L5101
// g5estrin  Estrin  Themla        L0101
// g4little  Little  Joyce Currie  L1010
// g5keller  Keller  Mary Kenneth  L5101
