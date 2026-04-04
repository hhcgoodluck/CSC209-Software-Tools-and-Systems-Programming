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
#include <signal.h>
#include <sys/wait.h>
#include "raid.h"

/*
 * This file implements the RAID controller that manages communication between
 * the main RAID simulator and the individual disk processes. It uses pipes
 * for inter-process communication (IPC) and fork to create child processes
 * for each disk.
 */

// Global array to store information about each disk's communication pipes.
static disk_controller_t* controllers;

static void log_cmd(const char *msg) {
    printf("[CMD] %s\n", msg);
    fflush(stdout);
}

static void log_flow(const char *msg) {
    printf("[FLOW] %s\n", msg);
    fflush(stdout);
}

static void log_info(const char *msg) {
    printf("[INFO] %s\n", msg);
    fflush(stdout);
}

static void log_error(const char *msg) {
    fprintf(stderr, "[ERROR] %s\n", msg);
    fflush(stderr);
}

/* Ignoring SIGPIPE allows us to check write calls for error rather than
 * terminating the whole system.
 */
static void ignore_sigpipe() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("Failed to set up SIGPIPE handler");
    }
}

/* Initialize the num-th disk controller, creating pipes to communicate
 * and creating a child process to handle disk requests.
 *
 * Returns 0 on success and -1 on failure.
 */
static int init_disk(int num) {
    ignore_sigpipe();

    if (pipe(controllers[num].to_disk) == -1) {
        perror("pipe to_disk");
        return -1;
    }

    if (pipe(controllers[num].from_disk) == -1) {
        perror("pipe from_disk");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        close(controllers[num].from_disk[0]);
        close(controllers[num].from_disk[1]);
        return -1;
    }

    /* child process */
    if (pid == 0) {

        /* child reads commands/data from parent on to_disk[0] */
        close(controllers[num].to_disk[1]);

        /* child writes results back to parent on from_disk[1] */
        close(controllers[num].from_disk[0]);

        /*
         * Close inherited pipe descriptors for previously created disks.
         * Since init_disk is called one disk at a time, this child inherits
         * all pipe fds the parent already had open.
         */
        int child_read_fd = controllers[num].to_disk[0];
        int child_write_fd = controllers[num].from_disk[1];

        for (int i = 0; i < num; i++) {
            if (controllers[i].to_disk[0] != child_read_fd &&
                controllers[i].to_disk[0] != child_write_fd) {
                close(controllers[i].to_disk[0]);
                }

            if (controllers[i].to_disk[1] != child_read_fd &&
                controllers[i].to_disk[1] != child_write_fd) {
                close(controllers[i].to_disk[1]);
                }

            if (controllers[i].from_disk[0] != child_read_fd &&
                controllers[i].from_disk[0] != child_write_fd) {
                close(controllers[i].from_disk[0]);
                }

            if (controllers[i].from_disk[1] != child_read_fd &&
                controllers[i].from_disk[1] != child_write_fd) {
                close(controllers[i].from_disk[1]);
                }
        }

        start_disk(num, child_write_fd, child_read_fd);

        /* start_disk should not return */
        close(controllers[num].to_disk[0]);
        close(controllers[num].from_disk[1]);
        _exit(EXIT_FAILURE);
    }

    /* parent process */
    controllers[num].pid = pid;

    /* parent writes to child on to_disk[1] */
    close(controllers[num].to_disk[0]);

    /* parent reads from child on from_disk[0] */
    close(controllers[num].from_disk[1]);

    return 0;
}


/* Restart the num-th disk, whose process is assumed to have already been killed.
 *
 * This function is very similar to init_disk.
 * However, since the other processes have all been started,
 * it needs to close a larger set of open pipe descriptors
 * inherited from the parent.
 *
 * Returns 0 on success and -1 on failure.
*/
int restart_disk(int num) {
    ignore_sigpipe();

    // Recreate the two pipes for disk num. The old process is already dead.
    if (pipe(controllers[num].to_disk) == -1) {
        perror("pipe to_disk");
        return -1;
    }

    if (pipe(controllers[num].from_disk) == -1) {
        perror("pipe from_disk");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(controllers[num].to_disk[0]);
        close(controllers[num].to_disk[1]);
        close(controllers[num].from_disk[0]);
        close(controllers[num].from_disk[1]);
        return -1;
    }

    /* child process */
    if (pid == 0) {

        // Keep only the ends this child needs.
        close(controllers[num].to_disk[1]);     // child does not write to to_disk
        close(controllers[num].from_disk[0]);   // child does not read from from_disk

        int child_read_fd = controllers[num].to_disk[0];
        int child_write_fd = controllers[num].from_disk[1];

        /*
         * Important difference from init_disk:
         * this child inherits *all* open pipe fds from the parent for every disk,
         * so close every unrelated pipe end.
         */
        int total_disks = num_disks + 1;
        for (int i = 0; i < total_disks; i++) {
            if (i == num) {
                continue;
            }

            if (controllers[i].to_disk[0] != child_read_fd &&
                controllers[i].to_disk[0] != child_write_fd) {
                close(controllers[i].to_disk[0]);
                }
            if (controllers[i].to_disk[1] != child_read_fd &&
                controllers[i].to_disk[1] != child_write_fd) {
                close(controllers[i].to_disk[1]);
                }
            if (controllers[i].from_disk[0] != child_read_fd &&
                controllers[i].from_disk[0] != child_write_fd) {
                close(controllers[i].from_disk[0]);
                }
            if (controllers[i].from_disk[1] != child_read_fd &&
                controllers[i].from_disk[1] != child_write_fd) {
                close(controllers[i].from_disk[1]);
                }
        }

        start_disk(num, child_write_fd, child_read_fd);

        /* start_disk should not return */
        close(child_read_fd);
        close(child_write_fd);
        _exit(EXIT_FAILURE);
    }

    /* parent process */
    controllers[num].pid = pid;

    /* Parent writes commands to to_disk[1] and reads results from from_disk[0]. */
    close(controllers[num].to_disk[0]);
    close(controllers[num].from_disk[1]);

    return 0;
}

/* Initialize all disk controllers by initializing the controllers
 * array and calling init_disk for each disk.
 *
 * total_disks is the number of data disks + 1 for the parity disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int init_all_controllers(int total_disks) {
    if (total_disks <= 0) {
        return -1;
    }

    controllers = malloc(sizeof(disk_controller_t) * total_disks);
    if (controllers == NULL) {
        perror("malloc");
        return -1;
    }

    // Initialize the controller array so cleanup / close logic is safer later.
    memset(controllers, 0, sizeof(disk_controller_t) * total_disks);

    for (int i = 0; i < total_disks; i++) {
        if (init_disk(i) == -1) {
            return -1;
        }
    }

    return 0;
}

/* Read the block of data at block_num from the appropriate disk.
 * The block is stored to the memory pointed to by data.
 *
 * If parity_flag == 1, read from parity disk.
 * If parity_flag == 0, read from data disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int read_block_from_disk(int block_num, char* data, int parity_flag) {
    // Validate the data buffer
    if (!data) {
        fprintf(stderr, "Error: Invalid data buffer\n");
        return -1;
    }

    // Identify the stripe to read from
    int disk_num;
    if (parity_flag == 1) {
        // Parity disk index(last)
        disk_num = num_disks;
    } else {
        // Data disk index
        disk_num = block_num % num_disks;
    }

    disk_command_t cmd = CMD_READ;

    // Each disk has a linear array of blocks, so the block number on an
    // individual disk is the same as the stripe number

    /* block number within an individual disk = stripe index */
    // block_num = block_num / num_disks;
    int disk_block_num = block_num / num_disks;

    // Write the command and the block number to the disk process
    // Then read the block from the disk process

    ssize_t n;
    int write_fd = controllers[disk_num].to_disk[1];

    /* Send read command */
    n = write(write_fd, &cmd, sizeof(cmd));
    if (n == -1) {
        restore_disk_process(disk_num);
        return -1;
    }
    if (n != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send read command to disk %d\n", disk_num);
        return -1;
    }

    /* Send block number within that disk */
    n = write(controllers[disk_num].to_disk[1], &disk_block_num, sizeof(disk_block_num));
    if (n == -1) {
        restore_disk_process(disk_num);
        return -1;
    }
    if (n != sizeof(disk_block_num)) {
        fprintf(stderr, "Error: Failed to send block number to disk %d\n", disk_num);
        return -1;
    }

    /* Read block_size bytes back from the child */
    size_t bytes_read_total = 0;
    while (bytes_read_total < (size_t)block_size) {
        n = read(controllers[disk_num].from_disk[0],
                 data + bytes_read_total,
                 block_size - bytes_read_total);

        if (n <= 0) {
            fprintf(stderr, "Error: Failed to read block data from disk %d\n", disk_num);
            return -1;
        }

        bytes_read_total += (size_t)n;
    }

    return 0;
}

/* Write a block of data to the block at block_num on the appropriate disk.
 * The block is stored at the memory pointed to by data.
 *
 * If parity_flag == 1, write to parity disk.
 * If parity_flag == 0, write to data disk.
 *
 * Returns 0 on success and -1 on failure.
 */
int write_block_to_disk(int block_num, char *data, int parity_flag) {
    if (!data) {
        fprintf(stderr, "Error: Invalid data buffer\n");
        return -1;
    }

    int disk_num;
    if (parity_flag == 1) {
        disk_num = num_disks;   // parity disk
    } else {
        disk_num = block_num % num_disks;
    }

    disk_command_t cmd = CMD_WRITE;

    /* block number within one individual disk = stripe index */
    int disk_block_num = block_num / num_disks;

    ssize_t n;

    /* Send command */
    n = write(controllers[disk_num].to_disk[1], &cmd, sizeof(cmd));
    if (n == -1) {
        restore_disk_process(disk_num);
        return -1;
    }
    if (n != sizeof(cmd)) {
        fprintf(stderr, "Error: Failed to send write command to disk %d\n", disk_num);
        return -1;
    }

    /* Send block number */
    n = write(controllers[disk_num].to_disk[1], &disk_block_num, sizeof(disk_block_num));
    if (n == -1) {
        restore_disk_process(disk_num);
        return -1;
    }
    if (n != sizeof(disk_block_num)) {
        fprintf(stderr, "Error: Failed to send block number to disk %d\n", disk_num);
        return -1;
    }

    /* Send block data */
    size_t bytes_written_total = 0;
    while (bytes_written_total < (size_t)block_size) {
        n = write(controllers[disk_num].to_disk[1],
                  data + bytes_written_total,
                  block_size - bytes_written_total);

        if (n == -1) {
            restore_disk_process(disk_num);
            return -1;
        }
        if (n == 0) {
            fprintf(stderr, "Error: Failed to write block data to disk %d\n", disk_num);
            return -1;
        }

        bytes_written_total += (size_t)n;
    }

    return 0;
}

/* Write the memory pointed to by data to the block at block_num on the
 * RAID system, handling parity updates.
 * If block_num is invalid (outside the range 0 to disk_size/block_size)
 * then return -1.
 *
 * Returns 0 on success and the disk number we tried to read from on failure.
 */
int write_block(int block_num, char *data) {
    if (data == NULL) {
        log_error("write_block received NULL data buffer");
        return -1;
    }

    int blocks_per_disk = disk_size / block_size;
    int total_logical_blocks = num_disks * blocks_per_disk;

    char msg[160];
    snprintf(msg, sizeof(msg), "wb %d <file>", block_num);
    log_cmd(msg);

    snprintf(msg, sizeof(msg), "Received write request for logical block %d", block_num);
    log_flow(msg);

    /* block_num refers only to logical data blocks, not the parity disk */
    if (block_num < 0 || block_num >= total_logical_blocks) {
        log_error("Invalid logical block number for write");
        return -1;
    }

    int data_disk_num = block_num % num_disks;
    int parity_disk_num = num_disks;

    snprintf(msg, sizeof(msg),
             "Logical block %d maps to data disk %d; parity disk is %d",
             block_num, data_disk_num, parity_disk_num);
    log_flow(msg);

    char *old_data = malloc(block_size);
    char *old_parity = malloc(block_size);
    if (old_data == NULL || old_parity == NULL) {
        perror("malloc");
        free(old_data);
        free(old_parity);
        log_error("Memory allocation failed during write_block");
        return -1;
    }

    /* Step 1: read old data block k */
    log_flow("Reading old data block");
    if (read_block_from_disk(block_num, old_data, 0) == -1) {
        free(old_data);
        free(old_parity);
        log_error("Failed to read old data block");
        return data_disk_num;
    }

    /* Step 2: read old parity block for k's stripe */
    log_flow("Reading old parity block");
    if (read_block_from_disk(block_num, old_parity, 1) == -1) {
        free(old_data);
        free(old_parity);
        log_error("Failed to read old parity block");
        return parity_disk_num;
    }

    /* Step 3 + 4: new_parity = old_parity XOR old_data XOR new_data */
    log_flow("Computing updated parity block");
    for (int i = 0; i < block_size; i++) {
        old_parity[i] ^= old_data[i];
        old_parity[i] ^= data[i];
    }

    /* Step 5: write updated parity block */
    log_flow("Writing updated parity block to parity disk");
    if (write_block_to_disk(block_num, old_parity, 1) == -1) {
        free(old_data);
        free(old_parity);
        log_error("Failed to write updated parity block");
        return parity_disk_num;
    }

    /* Step 6: write new data block */
    log_flow("Writing new data block to data disk");
    if (write_block_to_disk(block_num, data, 0) == -1) {
        free(old_data);
        free(old_parity);
        log_error("Failed to write new data block");
        return data_disk_num;
    }

    free(old_data);
    free(old_parity);

    snprintf(msg, sizeof(msg), "Write completed for logical block %d", block_num);
    log_info(msg);

    return 0;
}

/* Read the block at block_num from the RAID system into
 * the memory pointed to by data.
 * If block_num is invalid (outside the range 0 to disk_size/block_size)
 * then return NULL.
 *
 * Returns a pointer to the data buffer on success and NULL on failure.
 */
char *read_block(int block_num, char *data) {
    if (data == NULL) {
        log_error("read_block received NULL data buffer");
        return NULL;
    }

    int num_data_disks = num_disks;
    int blocks_per_disk = disk_size / block_size;
    int total_logical_blocks = num_data_disks * blocks_per_disk;

    char msg[128];
    snprintf(msg, sizeof(msg), "rb %d", block_num);
    log_cmd(msg);

    snprintf(msg, sizeof(msg), "Received read request for logical block %d", block_num);
    log_flow(msg);

    if (block_num < 0 || block_num >= total_logical_blocks) {
        log_error("Invalid logical block number for read");
        return NULL;
    }

    int data_disk_num = block_num % num_disks;
    int disk_block_num = block_num / num_disks;

    snprintf(msg, sizeof(msg),
             "Logical block %d maps to data disk %d, disk block %d",
             block_num, data_disk_num, disk_block_num);
    log_flow(msg);

    if (read_block_from_disk(block_num, data, 0) == -1) {
        snprintf(msg, sizeof(msg), "Read failed for logical block %d", block_num);
        log_error(msg);
        return NULL;
    }

    snprintf(msg, sizeof(msg), "Read completed for logical block %d", block_num);
    log_info(msg);

    return data;
}

/* Send exit command to all disk processes.
 *
 * Returns when all disk processes have terminated.
 */
void checkpoint_and_wait() {
    for (int i = 0; i < num_disks + 1; i++) {
        disk_command_t cmd = CMD_EXIT;
        ssize_t bytes_written = write(controllers[i].to_disk[1], &cmd, sizeof(cmd));
        if (bytes_written != sizeof(cmd)) {
            fprintf(stderr, "Warning: Failed to send exit command to disk %d\n", i);
        }
    }
    // wait for all disks to exit
    // we aren't going to do anything with the exit value
    for (int i = 0; i < num_disks + 1; i++) {
        wait(NULL);
    }
}

/* Simulate the failure of a disk by sending the SIGINT signal to the
 * process with id disk_num.
 */
void simulate_disk_failure(int disk_num) {
    char msg[128];

    snprintf(msg, sizeof(msg), "kill %d", disk_num);
    log_cmd(msg);

    snprintf(msg, sizeof(msg), "Sending SIGINT to disk %d", disk_num);
    log_flow(msg);

    if (kill(controllers[disk_num].pid, SIGINT) == -1) {
        perror("kill");
        snprintf(msg, sizeof(msg), "Failed to kill disk %d", disk_num);
        log_error(msg);
        return;
    }

    if (waitpid(controllers[disk_num].pid, NULL, 0) == -1) {
        perror("simulate_disk_failure: waitpid");
        snprintf(msg, sizeof(msg), "waitpid failed for disk %d", disk_num);
        log_error(msg);
        return;
    }

    snprintf(msg, sizeof(msg), "Disk %d has been terminated", disk_num);
    log_error(msg);
}

/* Restore the disk process after it has been killed.
 * If some aspect of restoring the disk process fails, 
 * then you can consider it a catastropic failure and 
 * exit the program.
 */
void restore_disk_process(int disk_num) {
    int blocks_per_disk = disk_size / block_size;

    /* Reap the dead child and close the parent's old pipe endpoints. */
    close(controllers[disk_num].to_disk[1]);
    close(controllers[disk_num].from_disk[0]);

    if (restart_disk(disk_num) == -1) {
        fprintf(stderr, "Fatal: failed to restart disk %d\n", disk_num);
        exit(EXIT_FAILURE);
    }

    char *recovered = malloc(block_size);
    char *temp = malloc(block_size);
    if (recovered == NULL || temp == NULL) {
        perror("malloc");
        free(recovered);
        free(temp);
        exit(EXIT_FAILURE);
    }

    for (int stripe = 0; stripe < blocks_per_disk; stripe++) {

        /*
         * Case 1: restoring a data disk.
         * recovered_block = parity_block XOR all other data blocks in the stripe
         */
        if (disk_num < num_disks) {
            int logical_block = stripe * num_disks + disk_num;

            if (read_block_from_disk(logical_block, recovered, 1) == -1) {
                fprintf(stderr, "Fatal: failed to read parity for stripe %d\n", stripe);
                free(recovered);
                free(temp);
                exit(EXIT_FAILURE);
            }

            for (int d = 0; d < num_disks; d++) {
                if (d == disk_num) {
                    continue;
                }

                int other_logical_block = stripe * num_disks + d;
                if (read_block_from_disk(other_logical_block, temp, 0) == -1) {
                    fprintf(stderr, "Fatal: failed to read data disk %d for stripe %d\n", d, stripe);
                    free(recovered);
                    free(temp);
                    exit(EXIT_FAILURE);
                }

                for (int i = 0; i < block_size; i++) {
                    recovered[i] ^= temp[i];
                }
            }

            if (write_block_to_disk(logical_block, recovered, 0) == -1) {
                fprintf(stderr, "Fatal: failed to rewrite recovered block to disk %d\n", disk_num);
                free(recovered);
                free(temp);
                exit(EXIT_FAILURE);
            }
        }

        /*
         * Case 2: restoring the parity disk.
         * parity_block = XOR of all data blocks in the stripe
         */
        else {
            memset(recovered, 0, block_size);

            for (int d = 0; d < num_disks; d++) {
                int logical_block = stripe * num_disks + d;

                if (read_block_from_disk(logical_block, temp, 0) == -1) {
                    fprintf(stderr, "Fatal: failed to read data disk %d for parity rebuild\n", d);
                    free(recovered);
                    free(temp);
                    exit(EXIT_FAILURE);
                }

                for (int i = 0; i < block_size; i++) {
                    recovered[i] ^= temp[i];
                }
            }

            /* Any block number from this stripe works with parity_flag == 1 */
            int stripe_logical_block = stripe * num_disks;
            if (write_block_to_disk(stripe_logical_block, recovered, 1) == -1) {
                fprintf(stderr, "Fatal: failed to rewrite recovered parity block\n");
                free(recovered);
                free(temp);
                exit(EXIT_FAILURE);
            }
        }
    }

    free(recovered);
    free(temp);
}

static int is_disk_alive(int disk_num) {
    pid_t pid = controllers[disk_num].pid;

    if (pid <= 0) {
        return 0;
    }

    if (kill(pid, 0) == -1) {
        return 0;
    }

    return 1;
}

void print_disk_status(void) {
    char msg[128];

    for (int i = 0; i < num_disks + 1; i++) {   // 如果最后一个是 parity disk
        pid_t pid = controllers[i].pid;
        const char *state = is_disk_alive(i) ? "ALIVE" : "DEAD";

        snprintf(msg, sizeof(msg),
                 "disk=%d state=%s pid=%d",
                 i, state, (int)pid);

        printf("[STATUS] %s\n", msg);
    }

    fflush(stdout);
}