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
    // Create two pipes
    int to_disk[2];   // Parent → Child: send commands and data
    int from_disk[2]; // Child → Parent: send results

    if (pipe(to_disk) == -1 || pipe(from_disk) == -1) {
        perror("pipe creation failed");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // Child process (disk)
        // Close unused pipe ends
        close(to_disk[1]);      // Child does not write to to_disk, only reads
        close(from_disk[0]);    // Child does not read from from_disk, only writes

        // Call start_disk to handle disk simulation
        start_disk(num, from_disk[1], to_disk[0]);

        // If start_disk returns, it means failure
        fprintf(stderr, "start_disk failed\n");
        exit(1);
    }

    // Parent process
    close(to_disk[0]);    // Parent does not read from to_disk
    close(from_disk[1]);  // Parent does not write to from_disk

    // Store pipe file descriptors and pid in controllers array
    controllers[num].to_disk[1] = to_disk[1];
    controllers[num].from_disk[0] = from_disk[0];
    controllers[num].pid = pid;

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
    // Avoid crash on broken pipe
  	ignore_sigpipe();

    // Close old pipe ends
    close(controllers[num].to_disk[1]);
    close(controllers[num].from_disk[0]);

    // Create new pipes
    int to_disk[2], from_disk[2];
    if (pipe(to_disk) == -1 || pipe(from_disk) == -1) {
        perror("pipe creation failed");
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        // Child process (disk)
        close(to_disk[1]);      // Child reads from to_disk[0]
        close(from_disk[0]);    // Child writes to from_disk[1]

        start_disk(num, from_disk[1], to_disk[0]);

        fprintf(stderr, "restart_disk: start_disk failed\n");
        exit(1);
    }

    // Parent updates controller information
    close(to_disk[0]);     // Parent writes to to_disk[1]
    close(from_disk[1]);   // Parent reads from from_disk[0]

    controllers[num].to_disk[1] = to_disk[1];
    controllers[num].from_disk[0] = from_disk[0];
    controllers[num].pid = pid;

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
  	// Avoid crash on broken pipe
  	ignore_sigpipe();

    // Allocate memory for the controllers array
    controllers = malloc(sizeof(disk_controller_t) * total_disks);
    if (!controllers) {
        perror("malloc failed");
        return -1;
    }

    // Initialize each disk (including the parity disk)
    for (int i = 0; i < total_disks; i++) {
        if (init_disk(i) == -1) {
            fprintf(stderr, "Failed to initialize disk %d\n", i);
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
    if (data == NULL) {
        fprintf(stderr, "Error: data buffer is NULL\n");
        return -1;
    }

    // Determine the disk number based on the parity flag
    int disk_num;
    if (parity_flag) {
        // Parity disk index
        disk_num = num_disks;
    } else {
        // Data disk index
        disk_num = block_num % num_disks;
    }

    // Adjust block number to match the appropriate stripe
    block_num = block_num / num_disks;
    disk_command_t cmd = CMD_READ;

    // Send the read command to the disk
    int write_fd = controllers[disk_num].to_disk[1];
    if (write(write_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) {
        restore_disk_process(disk_num);
        perror("Error sending read command to disk");
        return -1;
    }

    // Send the block number to the disk
    if (write(write_fd, &block_num, sizeof(block_num)) != sizeof(block_num)) {
        restore_disk_process(disk_num);
        perror("Error sending block number to disk");
        return -1;
    }

    // Read the block data from the disk and store it
    int read_fd = controllers[disk_num].from_disk[0];
    if (read(read_fd, data, block_size) != block_size) {
        perror("Error reading block data from disk");
        return -1;
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
    // Check if the provided data buffer is valid
  	if (data == NULL) {
        fprintf(stderr, "Error: data buffer is NULL\n");
        return -1;
    }

    // Determine the target disk based on the parity flag
    int disk_num;
    if (parity_flag == 1) {
        // Parity disk index
        disk_num = num_disks;
    } else {
        // Data disk index
        disk_num = block_num % num_disks;
    }

    // Adjust block number to match the appropriate stripe
    block_num = block_num / num_disks;
    disk_command_t cmd = CMD_WRITE;

    // Send write command to the disk
    if (write(controllers[disk_num].to_disk[1], &cmd, sizeof(cmd)) != sizeof(cmd)) {
        perror("Error sending write command to disk");
        restore_disk_process(disk_num);
        return -1;
    }

    // Send the block number to the disk
    if (write(controllers[disk_num].to_disk[1], &block_num, sizeof(block_num)) != sizeof(block_num)) {
        perror("Error sending block number to disk");
        restore_disk_process(disk_num);
        return -1;
    }

    // Write the block data to the disk
    if (write(controllers[disk_num].to_disk[1], data, block_size) != block_size) {
        perror("Error sending block data to disk");
        restore_disk_process(disk_num);
        return -1;
    }

    return 0;
}

/* Write the memory pointed to by data to the block at block_num on the
 * RAID system, handling parity updates.
 * If block_num is invalid (outside the range 0 to num_disks * disk_size/block_size - 1)
 * then return -1.
 *
 * Returns 0 on success and the disk number we tried to read from on failure.
 */
int write_block(int block_num, char *data) {
    // Validate block number
    if (block_num < 0 || block_num >= (num_disks * disk_size / block_size)) {
        return -1;
    }

    // Validate data buffer
    if (data == NULL) {
        fprintf(stderr, "Error: data buffer is NULL\n");
        return -1;
    }

    // Allocate memory for the original block and parity block
    char *old_data = malloc(block_size);
    char *parity = malloc(block_size);

    if (!old_data || !parity) {
        perror("malloc failed");
        free(old_data);
        free(parity);
        return -1;
    }

    // Index of the stripe containing the block
    int stripe_index = block_num / num_disks;

    // Index of the failed data disk
    int disk_index = block_num % num_disks;

    // Read the old data block
    if (read_block_from_disk(block_num, old_data, 0) == -1) {
        free(old_data);
        free(parity);
        return disk_index;  // failed to read from data disk
    }

    // Read the corresponding parity block
    if (read_block_from_disk(stripe_index, parity, 1) == -1) {
        free(old_data);
        free(parity);
        return num_disks;  // failed to read from parity disk
    }

    // Remove the old data from parity using XOR
    for (int i = 0; i < block_size; i++) {
        parity[i] ^= old_data[i];
    }

    // Add the new data into parity using XOR
    for (int i = 0; i < block_size; i++) {
        parity[i] ^= data[i];
    }

    // Write the updated parity block back to the parity disk
    if (write_block_to_disk(stripe_index, parity, 1) == -1) {
        free(old_data);
        free(parity);
        return num_disks;  // failed to write to parity disk
    }

    // Write the new data block to the data disk
    if (write_block_to_disk(block_num, data, 0) == -1) {
        free(old_data);
        free(parity);
        return disk_index;  // failed to write to data disk
    }

    // Free resources and return success
    free(old_data);
    free(parity);
    return 0;
}

/* Read the block at block_num from the RAID system into
 * the memory pointed to by data.
 * If block_num is invalid (outside the range 0 to num_disks * disk_size/block_size - 1)
 * then return NULL.
 *
 * Returns a pointer to the data buffer on success and NULL on failure.
 */
char *read_block(int block_num, char *data) {
    // Validate input arguments
    if (block_num < 0 || block_num >= (num_disks * disk_size / block_size)) {
        return NULL;
    }

	// Validate the data pointer
	if (!data) {
    	fprintf(stderr, "Error: data buffer is NULL\n");
    	return NULL;
	}

    // Attempt to read the block from the data disk
    if (read_block_from_disk(block_num, data, 0) == 0) {
        return data;
    }

    // Recovery mode — rebuild block using XOR if the data disk has failed
    int stripe_index = block_num / num_disks;     // Index of the stripe containing the block
    int failed_disk = block_num % num_disks;      // Index of the failed data disk

    // Initialize the recovery buffer with zeros
    memset(data, 0, block_size);

    // Temporary buffer for reading other blocks
    char *temp = malloc(block_size);
    if (!temp) {
        perror("malloc failed");
        return NULL;
    }

    // Flag to track if we successfully read a block
    int read_success = 0;

    // XOR all other data blocks in the stripe
    for (int i = 0; i < num_disks; i++) {
        if (i == failed_disk) continue;  // Skip the failed disk

        int logical_block = stripe_index * num_disks + i;
        if (read_block_from_disk(logical_block, temp, 0) == -1) {
            free(temp);
            fprintf(stderr, "Error: Failed to read data from disk %d\n", i);
            return NULL;
        }

        for (int j = 0; j < block_size; j++) {
            // XOR current block with the data buffer
            data[j] ^= temp[j];
        }

        // Mark when successfully read a block
        read_success = 1;
    }

    if (!read_success) {
        free(temp);
        fprintf(stderr, "Error: Unable to read enough data blocks for recovery\n");
        return NULL;
    }

    // XOR in the parity block
    if (read_block_from_disk(stripe_index, temp, 1) == -1) {
        free(temp);
        return NULL;
    }

    for (int j = 0; j < block_size; j++) {
        // XOR parity block into the recovery data
        data[j] ^= temp[j];
    }

    free(temp);
    return data;
}

/* Send exit command to all disk processes.
 *
 * Returns when all disk processes have terminated.
 */
void checkpoint_and_wait() {
    // Send CMD_EXIT to each disk (including the parity disk)
    for (int i = 0; i < num_disks + 1; i++) {
        disk_command_t cmd = CMD_EXIT;
        if (write(controllers[i].to_disk[1], &cmd, sizeof(cmd)) != sizeof(cmd)) {
            fprintf(stderr, "Warning: Failed to send exit command to disk %d\n", i);
        }
    }

    // Wait for all disk processes to terminate
    for (int i = 0; i < num_disks + 1; i++) {
        wait(NULL);
    }
}


/* Simulate the failure of a disk by sending the SIGINT signal to the
 * process with id disk_num.
 */
void simulate_disk_failure(int disk_num) {
    if (debug) {
        printf("Simulating failure: killing disk %d (pid = %d)\n",
               disk_num, controllers[disk_num].pid);
    }

    // Send SIGINT to the disk process
    if (kill(controllers[disk_num].pid, SIGINT) == -1) {
        perror("kill failed");
        exit(1);
    }

    // Wait for the disk process to terminate to avoid race condition
    if (waitpid(controllers[disk_num].pid, NULL, 0) == -1) {
        perror("waitpid failed");
        exit(1);
    }
}

/* Restore the disk process after it has been killed.
 * If some aspect of restoring the disk process fails, 
 * then you can consider it a catastropic failure and 
 * exit the program.
 */
void restore_disk_process(int disk_num) {
    if (debug) {
        fprintf(stderr, "Restoring disk process for disk %d\n", disk_num);
    }

    // Restart the disk process (child process)
    if (restart_disk(disk_num) == -1) {
        fprintf(stderr, "Error: Failed to restart disk %d\n", disk_num);
        exit(1);
    }

    // If the failed disk is the parity disk, then skip restoration.
    // Parity will be recomputed automatically when new data is written.
    if (disk_num == num_disks) {
        if (debug) {
            fprintf(stderr, "Parity disk %d restarted; no need to restore blocks.\n", disk_num);
        }
        return;
    }

    // Determine the number of blocks per disk
    int blocks_per_disk = disk_size / block_size;

    // Restore each block on the failed disk
    for (int block_num = 0; block_num < blocks_per_disk; block_num++) {
        // Compute the global block index for the current stripe
        int global_block_num = block_num * num_disks + disk_num;

        if (debug) {
            fprintf(stderr, "Restoring block %d (global index %d) on disk %d\n", block_num, global_block_num, disk_num);
        }

        // Allocate memory to store the recovered block and initialize it to zero
		char *recovered = malloc(block_size);
		if (!recovered) {
    		perror("malloc failed for recovered block");
    		exit(1);
		}
		memset(recovered, 0, block_size);

        // Read the parity block for the stripe
        char parity[block_size];
        if (read_block_from_disk(global_block_num, parity, 1) == -1) {
            perror("Failed to read parity block");
            free(recovered);
            exit(1);
        }

        // XOR all data blocks in the stripe, skipping the failed disk
        for (int i = 0; i < num_disks; i++) {
            if (i == disk_num) continue;  // Skip the failed disk

            int other_global_block = block_num * num_disks + i;
            char temp[block_size];

            // Read data from working disks
            if (read_block_from_disk(other_global_block, temp, 0) == -1) {
                perror("Failed to read data block from working disk");
                free(recovered);
                exit(1);
            }

            // XOR current data block with the recovered data
            for (int j = 0; j < block_size; j++) {
                recovered[j] ^= temp[j];
            }
        }

        // XOR in the parity block to get the missing data
        for (int j = 0; j < block_size; j++) {
            recovered[j] ^= parity[j];
        }

        // Write the reconstructed block back to the disk
        if (write_block_to_disk(global_block_num, recovered, 0) == -1) {
            perror("Failed to write reconstructed block to restored disk");
            free(recovered);
            exit(1);
        }

        if (debug) {
            fprintf(stderr, "Block %d restored successfully to disk %d\n", block_num, disk_num);
        }

        free(recovered);
    }

    if (debug) {
        fprintf(stderr, "Disk %d has been fully restored.\n", disk_num);
    }
}
