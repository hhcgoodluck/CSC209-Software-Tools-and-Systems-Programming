// Types and Type Conversions Video 3: Converting Strings to Integers

#include <stdio.h>
#include <stdlib.h>

int main() {
    //char *s = "17";
    char *s = "  -17";
    int i = strtol(s, NULL, 10);

    printf("i has the value %d\n", i);


    s = "  -17 other junk.";
    char *leftover;
    i = strtol(s, &leftover, 10);

    printf("i has the value %d\n", i);
    printf("leftover has the value %s\n", leftover);
    return 0;
}
