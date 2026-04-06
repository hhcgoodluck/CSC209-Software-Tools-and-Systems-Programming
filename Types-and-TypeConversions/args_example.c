// Types and Type Conversions Video 4: Command-line Arguments

#include <stdio.h>
// Try running this code from the commandline as ./a.out fee fi fo fum

int main(int argc, char **argv) {
    // argc is the number (or count) of arguments that were provided
    printf("%d\n", argc);

    // Notice the relationship of argv[0] to the executable.
    int i;
    for (i = 0; i < argc; i++ ) {
        printf("argv[%d] has the value %s\n", i , argv[i]);
    }
    return 0;
}
