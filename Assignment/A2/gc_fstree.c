#include <stdio.h>
#include "fstree.h"
#include "gc.h"

/* Traverse the fstree data-structure and call mark_one(void *vptr) on
 * each piece of memory that is still used in the fstree
 *
 * mark_one returns 0 if the chunk was marked for the first time and 1 if
 * it has been seen already. We need this to avoid revisiting pieces of the
 * tree that we have already marked -- where hard links have created cycles
 */

void mark_fstree(void *head) {
    Fstree *node = (Fstree *)head;
    if (node == NULL) return;        // 递归终止条件

    // Step 1: 标记当前 `Fstree` 结构体
    int already_marked = mark_one(node);
    if (already_marked) return;      // 已标记，避免死循环

    // Step 2: 标记 `node->name`（如果是动态分配的）
    if (node->name) {
        mark_one(node->name);
    }

    // Step 3: 遍历 `links` 链表，递归标记所有子目录
    Link *curr = node->links;
    while (curr != NULL) {
        // 标记 `Link` 结构体本身
        if (mark_one(curr) == 0) {    // 第一次标记
            mark_fstree(curr->fptr);  // 递归标记子目录
        }
        curr = curr->next;            // 继续遍历下一个 `Link`
    }

}
