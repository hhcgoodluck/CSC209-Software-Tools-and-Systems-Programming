#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fstree.h"
#include "gc.h"

void mark_fstree(void *head);


/* Perform the transactions from the open file fp, on the tree rooted at root.
 * The format of a transaction is illustrated in the sample transaction file.
 * If the path missing from the transaction then the path is the empty string
 * which means it starts from root.
 */
void do_transactions(FILE *fp, Fstree *root) {

    char line[MAX_LINE];
    char *dest;

    while((fgets(line, MAX_LINE, fp)) != NULL) {
        line[strlen(line)-1] = '\0';

        /* Tolerate empty lines in the transaction file. */
        if (line[0] == '\0') continue;

        /* the transaction file has the format
         *    cmd name path [dest]
         *  but the argument order is reversed for the function calls
         */
        char *cmd = strtok(line, " ");
        char *name = strtok(NULL, " ");
        char *path = strtok(NULL, " ");
        char empty[2] = "";
        if(path == NULL) {
            path = empty;
        }
        switch(cmd[0]) {
            case '#' :
            // this allows comments in the transaction file
            break;

            case 'a' :
                add_node(root, path, name);
            break;

            case 'r':
                remove_node(root, path, name);
            break;

            case 'h':
                dest = strtok(NULL, " ");
                if(dest == NULL) {
                    dest = empty;
                }
                add_hard_link(root, dest, path, name);
            break;

            case 'p':
                print_tree(root, 0);
            break;
            case 'g':
                mark_and_sweep(root, mark_fstree);
            break;
            default:
                fprintf(stderr, "ERROR: Unknown command %c\n", cmd[0]);
        }
    }
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: do_fstree filename\n");
        exit(1);
    }
    char *transfile = argv[1];
    FILE *fp = fopen(transfile, "r");
    if(fp == NULL) {
        perror("fopen");
        exit(1);
    }
    Fstree *root;
    root = create_node("root");

    do_transactions(fp, root);
    print_tree(root, 0);
    fclose(fp);

    // once you've implemented your garbage collector on fstree, you can
    // call it from here at various points to clean up the garbage

    // Set the root to NULL so that the garbage collector can clean up
    // all the memory
    root = NULL;
    mark_and_sweep(root, mark_fstree);

    return 0;
}
