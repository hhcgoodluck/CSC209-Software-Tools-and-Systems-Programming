// Types and Type Conversions Video 5: A Worked Example

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {

    // Ensure that the program was called correctly.
    if (argc < 3) {
        printf("Usage: sum_or_avg <operation s/a> <args ...>");
        return 1;
    }

    int total = 0;

    int i;
    for (i = 2; i < argc; i++) {
        total += strtol(argv[i], NULL, 10);
    }

    if (argv[1][0] == 'a') {
        // We need to cast total to float before the division.
        // Try removing the cast and see what happens.
        double average = (float) total / (argc - 2);
        printf("average: %f \n", average);
    } else {
        printf("sum: %d\n", total);
    }

    return 0;
}
