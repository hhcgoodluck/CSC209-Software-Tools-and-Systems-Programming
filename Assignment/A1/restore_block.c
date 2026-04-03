#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE 1024
#endif

/* These function signatures are needed so the compiler knows about them.
 */
int get_data(int num_blocks, int block_size, char data[][block_size],
             FILE *data_file);
void print_data_block(int block_num, int block_size, char data[][block_size]);
void print_data(int num_blocks, int block_size, char data[][block_size]);

void compute_parity_block(unsigned char *parity_block, int num_blocks, 
                    int block_size, char data[][block_size]);
void delete_data_block(int block_num, int block_size, char data[][block_size]);
void reconstruct_block(int lost_block, int num_blocks, int block_size, 
                       char data[][block_size], unsigned char *parity_block);

/* This program is similar to compute_parity.  It also deletes a block from
 * data by setting its values to '\0', and then uses the parity block and the
 * other data blocks to restore it.
 */
int main(int argc, char **argv) {

    if (argc != 5) {
        printf("Usage: restore_block num_blocks block_size filename lost_block\n");
        printf("       num_blocks: number of blocks to compute the parity over\n");
        printf("       block_size: size of each block\n");
        printf("       filename: file containing the data blocks\n");
        printf("       lost_block: the block number to be restored\n");
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
    int lost_block = (int)strtol(argv[4], NULL, 10);

    char data[num_blocks][block_size];

    if (get_data(num_blocks, block_size, data, data_file) != 0) {
        fprintf(stderr, "Error reading data from file\n");
        exit(1);
    }
    fclose(data_file);

    // Initialize the parity block
    unsigned char parity_block[block_size];
    for(int i = 0; i < block_size; i++) {
        parity_block[i] = 0;
    }

    compute_parity_block(parity_block, num_blocks, block_size, data);

    printf("Data before block %d is deleted\n", lost_block);
    print_data(num_blocks, block_size, data);
    printf("\n");

    delete_data_block(lost_block, block_size, data);

    printf("Data after block %d is deleted\n", lost_block);
    print_data(num_blocks, block_size, data);
    printf("\n");

    reconstruct_block(lost_block, num_blocks, block_size, data, parity_block);
    printf("Reconstructed block %d:", lost_block);
    print_data_block(lost_block, block_size, data);
    return 0;
}
