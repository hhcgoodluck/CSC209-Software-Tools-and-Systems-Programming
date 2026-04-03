/* This code is provided solely for the personal and private use of students
 * taking the CSC209H course at the University of Toronto. Copying for purposes
 * other than this use is expressly prohibited. All forms of distribution of
 * this code, including but not limited to public repositories on GitHub,
 * GitLab, Bitbucket, or any other online platform, whether as given or with
 * any changes, are expressly prohibited.
 *
 * Authors: Karen Reid, Paul He, Philip Kukulak
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2025 Karen Reid
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raid.h"


int debug = 1;  // Set to 1 to enable debug output, 0 to disable
static int checkpoint_disk(char *disk_data, int id);

/*
 * Main function for the disk simulation process, which runs in a child process
 * created by the RAID controller.
 *
 * id is the disk number or index into the controllers table,
 * to_parent is the pipe descriptor for writing to the parent,
 * from_parent is the pipe descriptor for reading from the parent.
 *
 * Returns 0 on success and 1 on failure.
 */
int start_disk(int id, int to_parent, int from_parent) {
    int status = 0;

    // Allocate memory for this disk's data
    char *disk_data = malloc(disk_size);
    if (!disk_data) {
        perror("Failed to allocate memory for disk");
        return 1;
    }

    // Initialize disk to zero (simulating a fresh disk)
    memset(disk_data, 0, disk_size);

    // Enter the main loop, where the disk will read and execute commands.
    while (1) {
        disk_command_t cmd;

        // Read a command from the controller (parent process)
        if (read(from_parent, &cmd, sizeof(cmd)) != sizeof(cmd)) {
            fprintf(stderr, "Disk %d: Failed to read command\n", id);
            status = 1;
            break;
        }

        switch (cmd) {
            case CMD_READ: {
                int block_index;

                // Read the block index to determine which block to read.
                if (read(from_parent, &block_index, sizeof(block_index)) != sizeof(block_index)) {
                    fprintf(stderr, "Disk %d: Failed to read block index for READ\n", id);
                    status = 1;
                    break;
                }

                // Calculate the offset based on the block index.
                int offset = block_index * block_size;

                // Ensure the offset and block size are within valid limits.
                if (offset + block_size > disk_size) {
                    fprintf(stderr, "Disk %d: Invalid READ offset\n", id);
                    status = 1;
                    break;
                }

                // Write the block data back to the parent process.
                if (write(to_parent, disk_data + offset, block_size) != block_size) {
                    fprintf(stderr, "Disk %d: Failed to write block back for READ\n", id);
                    status = 1;
                }
                break;
            }

            case CMD_WRITE: {
                int block_index;

                // Read the block index to determine which block to write.
                if (read(from_parent, &block_index, sizeof(block_index)) != sizeof(block_index)) {
                    fprintf(stderr, "Disk %d: Failed to read block index for WRITE\n", id);
                    status = 1;
                    break;
                }

                // Calculate the offset based on the block index.
                int offset = block_index * block_size;

                // Ensure the offset and block size are within valid limits.
                if (offset + block_size > disk_size) {
                    fprintf(stderr, "Disk %d: Invalid WRITE offset\n", id);
                    status = 1;
                    break;
                }

                // Read the data to be written and store it in the disk data array.
                if (read(from_parent, disk_data + offset, block_size) != block_size) {
                    fprintf(stderr, "Disk %d: Failed to read data for WRITE\n", id);
                    status = 1;
                }
                break;
            }

            case CMD_EXIT:
                // If the exit command is received, save the disk data and exit.
                if (debug) {
                    printf("Disk %d: Received EXIT command. Saving to file and exiting\n", id);
                }
                if (checkpoint_disk(disk_data, id) == -1) {
                    fprintf(stderr, "Disk %d: Failed to checkpoint on exit\n", id);
                    status = 1;
                }
                break;

            default:
                // Handle invalid or unknown commands.
                fprintf(stderr, "Disk %d: Unknown command %d received\n", id, cmd);
                status = 1;
                break;
        }

        // Break loop if any error occurred or exit command received
        if (cmd == CMD_EXIT || status) {
            break;
        }
    }

    // Cleanup to free the allocated memory and exit.
    free(disk_data);
    exit(status);
}

/* Save the disk's data, pointed to by disk_data, to a file named id.
 *
 * Returns 0 on success, and -1 on failure.
 */
static int checkpoint_disk(char *disk_data, int id) {
    if (!disk_data) {
        fprintf(stderr, "Error: Invalid parameters for checkpoint\n");
        return -1;
    }

    // Create a file name for this disk
    char disk_name[MAX_NAME];
    if (snprintf(disk_name, sizeof(disk_name), "disk_%d.dat", id) >= (int)sizeof(disk_name)) {
        fprintf(stderr, "Error: Disk name too long for disk %d\n", id);
        return 1;
    }

    FILE *fp = fopen(disk_name, "wb");
    if (!fp) {
        perror("Failed to create checkpoint file");
        return -1;
    }

    size_t bytes_written = fwrite(disk_data, 1, disk_size, fp);
    if (bytes_written != (size_t)disk_size) {
        if (ferror(fp)) {
            fprintf(stderr, "Failed to write checkpoint data");
        } else {
            fprintf(stderr, "Error: Incomplete write during checkpoint\n");
        }
        fclose(fp);
        return -1;
    }

    if (fclose(fp) != 0) {
        perror("Failed to close checkpoint file");
        return -1;
    }

    return 0;
}
