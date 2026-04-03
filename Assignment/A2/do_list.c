/* Read and execute a list of operations on a linked list.
 * Periodically call the garbage collector.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "gc.h"

#define MAX_LINE 128
#define ADD_NODE 'a'
#define DEL_NODE 'r'
#define GC 'g'
#define PRINT_LIST 'p'
#define COMMENT '#'

void mark_list(void *); 


int main(int argc, char **argv) {
    List *list_head;

    char line[MAX_LINE];
    char *str;

    if(argc != 2) {
        fprintf(stderr, "Usage: do_list filename\n");
        exit(1);
    }

    FILE *fp;
    if((fp = fopen(argv[1], "r")) == NULL) {
        perror("fopen");
        exit(1);
    }

    list_head = NULL;

    while(fgets(line, MAX_LINE, fp) != NULL) {

        char *next;
        int value;
        int type = line[0];
        next = line + 2;

        switch(type) {
            case ADD_NODE :
                value = strtol(next, NULL, 0);           
                list_head = add_node(list_head, value);
                break;
            case DEL_NODE :
                value = strtol(next, NULL, 0);
                list_head = remove_node(list_head, value);
                break;
            case PRINT_LIST :
                str = tostring(list_head);
                printf("List is %s\n", str);
                free(str);
                break;
            case GC :
                mark_and_sweep(list_head, mark_list);
                break;
            case COMMENT :
                // ignore comments
                break;
            default :
                fprintf(stderr, "Error: bad transaction type\n");

        }
    }
    fclose(fp);

    // This should free all allocated memory
    list_head = NULL;
    mark_and_sweep(list_head, mark_list);

    return 0;
}
