#ifndef __GC_H__
#define __GC_H__

/* External functions that are the interface to the garbage collector */

#ifndef LOGFILE
#define LOGFILE "gc.log"
#endif

/* Normally this global variable would not be accessible outside
 * the gc.c file, but for testing your work on MarkUs, we want to
 * expose it.
 */
#ifdef TESTING
extern struct mem_chunk *memory_list_head;
#endif

extern int debug;

#define NOT_USED 0
#define USED 1

struct mem_chunk {
    int in_use;
    void *address;
    struct mem_chunk *next;
};

/* A wrapper around malloc to keep track of dynamically allocated memory
 */
void *gc_malloc(int nbytes);

/* Executes the garbage collector.
 * mark_obj will traverse the data structure rooted at obj, calling
 * mark_one for each dynamically allocated chunk of memory to mark
 * that it is still in use.
 */
void mark_and_sweep(void *obj, void (*mark_obj)(void *));

/* Mark vptr as still in use
 * Return code:
 *   0 on success
 *   1 if memory chunk pointed to by vptr was already marked as in use
 *   2 if memory chunk pointed to by vptr was not found in memory list
 */
int mark_one(void *vptr);

#endif // __GC_H__
