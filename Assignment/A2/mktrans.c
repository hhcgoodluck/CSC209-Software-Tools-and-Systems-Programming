/*
 * Generate a random series of add, delete, and print operations on a 
 * linked list.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

#define MAX_VAL 10000

/* The probability of generating print and add operations
 * The probability of generating a garbage collection operation is 
 * 1 - (PROB_PRINT + PROB_ADD + PROB_DEL)
 */
#define PROB_PRINT 0.05
#define PROB_ADD 0.55
#define PROB_DEL 0.35

int main(int argc, char **argv) {
    char type;
    long value;
    int length = 0;
    List *ll;

    FILE *fp;
    if (argc > 1) {
        if ((fp = fopen(argv[1], "w")) == NULL) {
            perror(argv[1]);
            exit(1);
        }
    } else {
        fp = stdout;
    }
    int i;
    for(i = 0; i < MAX_VAL; i++) {
        double prob = (double)random() / RAND_MAX;
        if(prob < PROB_PRINT) {
            type = 'p';
            fprintf(fp,"%c\n", type);
        } else if(prob < PROB_PRINT + PROB_ADD) {
            type = 'a';
            value = (int)(random() % MAX_VAL);
            ll = add_node(ll, value);
            length++;
            fprintf(fp,"%c %ld\n", type, value);
        } else if(prob < PROB_PRINT + PROB_ADD + PROB_DEL) {
            type = 'r';
            if(length > 0) {
                // choose a node that is in the list to delete
                int index = (int)(random() % length);
                value = find_nth(ll, index);
                ll = remove_node(ll, value);
                length--;
                fprintf(fp,"%c %ld\n", type, value);
            } 
        } else {
            type = 'g';
            fprintf(fp,"%c\n", type);
        }
    }
    return 0;
}


