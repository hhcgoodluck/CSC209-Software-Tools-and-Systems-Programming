// Types and Type Conversions Video 2: Casting

#include <stdio.h>

int main() {
    // Before running this code, predict what it will print. Then
    // run it to see if your prediction matches what actually happens.

    int i = 5;
    int j = 10;

    int k = i / j;
    printf("int k is %d\n", k);

    double half = 0.5;
    k = half;
    printf("int k is %d\n", k);

    double d = i / j;
    printf("double d is %f\n", d);

    d = (double) i / j;
    printf("double d is %f\n", d);

    return 0;
}
