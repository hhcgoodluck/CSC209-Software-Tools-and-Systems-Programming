#include <stdio.h>
#include <stdlib.h>

/*
Write a small program to use scanf to read two values from standard input. The first is a
10 character string and the second is an integer. The program takes no command-line arguments.

If the integer is -1, the program prints the full string to stdout. If the integer is between 0 and 9 inclusive
the program prints only the corresponding digit from the string to stdout. In both of these cases the
program returns 0. If the integer is less than -1 or greater than 9, the program prints the message "ERROR"
to stdout and returns 1.

 */
int main() {
    char phone[11];
    int index;

    scanf("%10s %d", phone, &index);

    if (index == -1) {
        printf("%s\n", phone);
        return 0;
    } else if (index >= 0 && index <= 9) {
        printf("%c\n", phone[index]);
        return 0;
    } else {
        printf("ERROR\n");
        return 1;
    }
}


