#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // Only used for reading from the file.

#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE 1024
#endif

/* These function signatures are needed so the compiler knows about them.
 * We will cover header files later in the term as an alterative to this.
 */
int get_data(int num_blocks, int block_size, char data[][block_size],
             FILE *data_file);
void print_data_block(int block_num, int block_size, char data[][block_size]);
void print_data(int num_blocks, int block_size, char data[][block_size]);
void print_parity_block(int block_size, char *parity_block);
void compute_parity_block(char *parity_block, int num_blocks, 
                    int block_size, char data[][block_size]);


int main(int argc, char **argv) {

    if (argc != 4) {
        printf("Usage: compute_parity num_blocks block_size filename\n");
        printf("       num_blocks: number of blocks to compute the parity over\n");
        printf("       block_size: size of each block\n");
        printf("       filename: file containing the data blocks\n");
        return 0;
    }

    // Process the command line arguments
    int num_blocks = (int)strtol(argv[1], NULL, 10);
    int block_size = (int)strtol(argv[2], NULL, 10);
    if (block_size <= 0 || block_size > MAX_BLOCK_SIZE) {
        printf("The block size should be a positive integer less than %d.\n", MAX_BLOCK_SIZE);
        return 0;
    }
    char *file_name = argv[3];
    FILE *data_file = fopen(file_name, "r");
    if (data_file == NULL) {
        perror("fopen");
        exit(1);
    }

    char data[num_blocks][block_size];

    if (get_data(num_blocks, block_size, data, data_file) != 0) {
        // termianate the program if there is an error reading the data
        fprintf(stderr, "Error reading data from file\n");
        exit(1);
    }

    // We don't need the file anymore, so close it 
    fclose(data_file);

    // Print the data blocks so that we can see what they look like
    printf("Data blocks:\n");
    print_data(num_blocks, block_size, data);

    // Initialize the parity block
    char parity_block[block_size];
    for(int i = 0; i < block_size; i++) {
        parity_block[i] = 0;
    }

    compute_parity_block(parity_block, num_blocks, block_size, data);

    printf("Parity block:\n");
    print_parity_block(block_size, parity_block);

    return 0;
}
