#include <stdio.h>
#include "list.h"
#include "gc.h"

/* traverse the list data-structure and call mark_one(void *vptr) on
 *  each piece of memory that is still used in the list
 * head is the head of the list data-structure being traversed NOT
 * the head of the memory-list which is part of gc
 */
void mark_list(void *head) {
    List *curr = (List *)head;
    while (curr != NULL) {
        mark_one(curr);     // 标记当前节点
        curr = curr->next;  // 继续遍历下一个节点
    }
}
