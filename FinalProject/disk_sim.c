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

static int checkpoint_disk(char *disk_data, int id);
int debug = 1;  // Set to 1 to enable debug output, 0 to disable

/* Read exactly count bytes unless EOF/error happens.
 * Returns number of bytes read on success, -1 on error.
 */
static ssize_t read_full(int fd, void *buf, size_t count) {
    size_t total = 0;
    char *p = buf;

    while (total < count) {
        ssize_t n = read(fd, p + total, count - total);
        if (n < 0) {
            return -1;   // real error
        }
        if (n == 0) {
            break;       // EOF
        }
        total += (size_t)n;
    }

    return (ssize_t)total;
}

/* Write exactly count bytes unless error happens.
 * Returns number of bytes written on success, -1 on error.
 */
static ssize_t write_full(int fd, const void *buf, size_t count) {
    size_t total = 0;
    const char *p = buf;

    while (total < count) {
        ssize_t n = write(fd, p + total, count - total);
        if (n < 0) {
            return -1;
        }
        if (n == 0) {
            return -1;
        }
        total += (size_t)n;
    }
    return (ssize_t)total;
}


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
    int running = 1;

    char *disk_data = calloc((size_t)disk_size, 1);
    if (disk_data == NULL) {
        perror("calloc");
        return 1;
    }

    while (running) {
        disk_command_t cmd;

        ssize_t n = read_full(from_parent, &cmd, sizeof(cmd));
        if (n < 0) {
            perror("read_full command");
            fprintf(stderr, "Error: disk %d read error on fd %d\n", id, from_parent);
            status = 1;
            break;
        }
        if (n == 0) {
            fprintf(stderr, "Disk %d: EOF on command pipe fd %d\n", id, from_parent);
            break;
        }
        if (n != (ssize_t)sizeof(cmd)) {
            fprintf(stderr,
                    "Error: disk %d partial command read: got %zd bytes, expected %zu\n",
                    id, n, sizeof(cmd));
            status = 1;
            break;
        }

        switch (cmd) {
            case CMD_READ: {
                int block_num;
                int blocks_per_disk = disk_size / block_size;

                if (read_full(from_parent, &block_num, sizeof(block_num)) !=
                    (ssize_t)sizeof(block_num)) {
                    fprintf(stderr, "Error: disk %d failed to read block number for CMD_READ\n", id);
                    status = 1;
                    break;
                }

                if (block_num < 0 || block_num >= blocks_per_disk) {
                    fprintf(stderr, "Error: disk %d got invalid block number %d for CMD_READ\n",
                            id, block_num);
                    status = 1;
                    break;
                }

                char *block_addr = disk_data + ((size_t)block_num * (size_t)block_size);
                if (write_full(to_parent, block_addr, (size_t)block_size) !=
                    (ssize_t)block_size) {
                    fprintf(stderr, "Error: disk %d failed to return block data for CMD_READ\n", id);
                    status = 1;
                }
                break;
            }

            case CMD_WRITE: {
                int block_num;
                int blocks_per_disk = disk_size / block_size;

                if (read_full(from_parent, &block_num, sizeof(block_num)) !=
                    (ssize_t)sizeof(block_num)) {
                    fprintf(stderr, "Error: disk %d failed to read block number for CMD_WRITE\n", id);
                    status = 1;
                    break;
                }

                if (block_num < 0 || block_num >= blocks_per_disk) {
                    fprintf(stderr, "Error: disk %d got invalid block number %d for CMD_WRITE\n",
                            id, block_num);
                    status = 1;
                    break;
                }

                char *block_addr = disk_data + ((size_t)block_num * (size_t)block_size);
                if (read_full(from_parent, block_addr, (size_t)block_size) !=
                    (ssize_t)block_size) {
                    fprintf(stderr, "Error: disk %d failed to read block data for CMD_WRITE\n", id);
                    status = 1;
                }
                break;
            }

            case CMD_EXIT:
                running = 0;
                break;

            default:
                fprintf(stderr, "Error: Unknown command %d received\n", cmd);
                status = 1;
                break;
        }

        if (status != 0) {
            break;
        }
    }

    if (checkpoint_disk(disk_data, id) == -1) {
        status = 1;
    }

    free(disk_data);
    close(to_parent);
    close(from_parent);

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
        return -1;
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
