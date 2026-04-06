// Types and Type Conversions Video 1: Numeric Types

#include <stdio.h>
#include <limits.h>

int main() {

    // Create an int and a double and just print them.
    int i = 17;
    double d = 4.8;
    printf("i is %d\n", i);
    printf("d is %f\n", d);

    // What happens when we assign a double to an int?
    i = d;
    printf("i is %d\n", i);
    // Mostly it just drops the fractional piece,
    // unless the double value is outside the range of the integer.
    printf("but if double is bigger than the largest int\n");
    d = 3e10;
    printf("d is %f\n", d);
    i = d;
    printf("i is %d\n", i);

    // What about assigning an integral type to a floating point type?
    i = 17;
    d = i;
    printf("d is %f\n", d);

    // What if it is a large unsigned int
    // and we assign it to a floating point type of the same size (in bytes)?

    printf("An integer is stored using %lu bytes \n", sizeof(i));
    printf("A double is stored using %lu bytes \n", sizeof(d));
    float f;
    printf("A float is stored using %lu bytes \n", sizeof(f));

    int big = INT_MAX;
    printf("big is %d uses %lu bytes \n", big, sizeof(big));
    f = big;
    printf("f is %f uses %lu bytes \n", f, sizeof(f));

    // Conversion between sizes of integral types.
    char ch = 'A';
    printf("char ch: displayed as char %c, displayed as int %d\n", ch, ch);

    int j = ch;
    printf("j is %c, int %d\n", j, j);

    i = 320;
    ch = i;
    printf("char ch: displayed as char %c, displayed as int %d\n", ch, ch);

    return 0;
}
