#include <stdio.h>
#include <stdlib.h>
#include "gc.h"

// global variable to be head of allocated pieces
struct mem_chunk *memory_list_head = NULL;

// global variable for debugging
int debug = 0;

void *gc_malloc(int nbytes) {
    void *ptr = malloc(nbytes);  // 分配内存
    if (!ptr) return NULL;       // 分配失败

    // 创建一个新的内存追踪块
    struct mem_chunk *new_chunk = (struct mem_chunk *)malloc(sizeof(struct mem_chunk));
    if (!new_chunk) {
        free(ptr); // 释放已分配的内存
        return NULL;
    }

    new_chunk->address = ptr;
    new_chunk->in_use = 1;              // 标记该块为使用状态
    new_chunk->next = memory_list_head; // 头插法插入到链表
    memory_list_head = new_chunk;

    return ptr;
}

/* Executes the garbage collector.
 * obj is the root of the data structure to be traversed
 * mark_obj is a function that will traverse the data structure rooted at obj
 *
 * The function will also write to the LOGFILE the results messages with the
 * following format strings:
 * "Mark_and_sweep running\n"
 * "Chunks freed this pass: %d\n"
 * "Chunks still allocated: %d\n"
 */

void mark_and_sweep(void *obj, void (*mark_obj)(void *)) {

    // 打开 LOGFILE 以追加模式写入日志
    FILE *logfile = fopen(LOGFILE, "a");
    if (!logfile) {
        fprintf(stderr, "ERROR: Could not open log file %s\n", LOGFILE);
        return;
    }
    fprintf(logfile, "Mark_and_sweep running\n");

    // Step 1: 将所有 in_use 置为 0
    struct mem_chunk *curr = memory_list_head;
    while (curr != NULL) {
        curr->in_use = 0;
        curr = curr->next;
    }

    // Step 2: 标记仍在使用的对象
    if (obj != NULL) {  // 检查 obj 是否为空
        mark_obj(obj);
    }

    // Step 3: 释放未使用的内存
    struct mem_chunk **indirect = &memory_list_head;
    int freed_count = 0, allocated_count = 0;

    while (*indirect != NULL) {
        curr = *indirect;
        if (curr->in_use == 0) {
            *indirect = curr->next;
            free(curr->address);
            free(curr);
            freed_count++;
        } else {
            allocated_count++;
            indirect = &curr->next;
        }
    }

    // 写入日志
    fprintf(logfile, "Chunks freed this pass: %d\n", freed_count);
    fprintf(logfile, "Chunks still allocated: %d\n", allocated_count);
    fclose(logfile);

}


/*
 Mark the chunk of memory pointed to by vptr as in use.
 Return codes:
   0 if successful
   1 if chunk already marked as in use
   2 if chunk is not found in memory list

   Here is a print statement to print an error message:
   fprintf(stderr, "ERROR: mark_one address not in memory list\n");
 */
int mark_one(void *vptr) {
    struct mem_chunk *curr = memory_list_head;
    while (curr != NULL) {
        if (curr->address == vptr) {
            if (curr->in_use == 1) {
                return 1; // 该内存块已经被标记
            }
            curr->in_use = 1;
            return 0; // 成功标记
        }
        curr = curr->next;
    }

    if (debug) {  // 在 debug 模式下打印
        fprintf(stderr, "ERROR: mark_one address not in memory list\n");
    }
    return 2; // 未找到该内存块
}

void print_memory_list() {
    struct mem_chunk *current = memory_list_head;
    while (current != NULL) {
        printf("%lx (%d) -> ",(unsigned long) current->address, current->in_use);
        current = current->next;
    }
    printf("\n");
}
