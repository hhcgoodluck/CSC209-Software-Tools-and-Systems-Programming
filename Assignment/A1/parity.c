#include <stdio.h>  // 标准输入输出（printf()，scanf()，fopen() 等）
#include <stdlib.h> // 标准库（malloc()，free()，exit() 等）
#include <string.h> // only used for reading from the file  字符串处理（strcpy()，strlen()，strcmp() 等）


#ifndef MAX_BLOCK_SIZE
#define MAX_BLOCK_SIZE 1024
#endif


/* Build a 2D array of data by reading it from the open file data_file. 
 * Each line in the file represents a "block" of data and will be stored
 * as a row in the 2D array.
 *  - num_blocks specifies the number of lines to read from data_file
 *    and corrsponds to the number of rows in the 2D array.
 *  - block_size specifies the number of characters or bytes in each block.
 *    Note that this may not match the number of characters in the lines 
 *    of the file.
 *  - data is the 2D array that will store the data read from the file.
 *    It has num_blocks rows and block_size columns.
 *  - data_file is the file to read the data from. It has already
 *    been opened for reading.
 * 
 * Note that the rows in data are not null-terminated.  We are using data as
 * a 2D array of bytes, not a 2D array of strings and will not be using string
 * functions on it.
*/

int get_data(int num_blocks, int block_size, char data[][block_size],
             FILE *data_file) {

    char buffer[MAX_BLOCK_SIZE];

    for (int i = 0; i < num_blocks; i++) {
        // Read a full line from the file.  We assume that no line is longer
        // than MAX_BLOCK_SIZE characters.
        if (fgets(buffer, MAX_BLOCK_SIZE, data_file) == NULL) {
            fprintf(stderr, "Error reading data from file\n");
            return 1;
        }
        for (int j = 0; j < block_size; j++) {
            // if the line didn't have enough data, pad with 0
            if (j >= strlen(buffer)) {
                data[i][j] = 0;
            } else{
                data[i][j] = buffer[j];
            }
        }
    }
    return 0;
}


/* Print to standard output the row of data at index block_num 
 * Print the data as characters using the %c format specifier 
 * for easier reading. String functions will not work to print
 * the data because the data is not null-terminated.
 */
void print_data_block(int block_num, int block_size, char data[][block_size]) {
    for (int i = 0; i < block_size; i++) {
        printf("%c", data[block_num][i]);
    }
    printf("\n");
}


/* Print the full 2d array of data to standard output.
 * This function is primarily used for testing and debugging.
 * Use print_data_block to print each row of data.
 */
void print_data(int num_blocks, int block_size, char data[][block_size]) {
    for (int i = 0; i < num_blocks; i++) {
        print_data_block(i, block_size, data);
    }
}


/* Construct a parity block by xoring all the data blocks together.
 * - parity_block is the array that will store the parity data.
 * - num_blocks is the number of blocks in the data array.
 * - block_size is the number of bytes in each block.
 * - data is the 2D array of data blocks.
 */

void compute_parity_block(char *parity_block, int num_blocks, 
                    int block_size, char data[][block_size]) {
    for (int j = 0; j < block_size; j++) {
        parity_block[j] = 0;
    }

    for (int j = 0; j < block_size; j++) {
        for (int i = 0; i < num_blocks; i++) {
            parity_block[j] ^= data[i][j];
        }
    }
}


/* Print the parity block as unsigned hexadecimal numbers.
 * We are printing the values as hexadecimal numbers because the parity values
 * may not be printable characters and each byte or character is 2 hexadecimal
 * digits which makes it easier to read.
 * Use the format string "%02x " to print each byte.
 */
void print_parity_block(int block_size, unsigned char *parity_block) {
    for (int i = 0; i < block_size; i++) {
        printf("%02x ", parity_block[i]);
    }
    printf("\n");
}


/* Delete the data in the block at index block_num by setting all the 
 * bytes to '\0' (or 0).
 */
void delete_data_block(int block_num, int block_size, char data[][block_size]) {
    for (int j = 0; j < block_size; j++) {
        data[block_num][j] = '\0';
    }
}

/* Reconstruct the block in data at index lost_block by xoring the remaining
 * data blocks with the parity block.
 * - lost_block is the index of the block to reconstruct.
 * - num_blocks is the number of blocks in the data array.
 * - block_size is the number of bytes in each block.
 * - data is the 2D array of data blocks.
 * - parity_block is the parity block.
 */

void reconstruct_block(int lost_block, int num_blocks, int block_size, 
                       char data[][block_size], char *parity_block) {
    for (int j = 0; j < block_size; j++) {
        data[lost_block][j] = 0;
    }

    for (int j = 0; j < block_size; j++) {
        data[lost_block][j] = parity_block[j];
        for (int i = 0; i < num_blocks; i++) {
            if (i != lost_block) {
                data[lost_block][j] ^= data[i][j];
            }
        }
    }
}