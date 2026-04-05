// Strings Video 3: Size and Length

#include <stdio.h>
#include <string.h>

int main() {
    char weekday[10] = "Monday";
    printf("Size of string: %lu\n", sizeof(weekday));
    printf("Length of string: %lu\n", strlen(weekday));
    return 0;
}
