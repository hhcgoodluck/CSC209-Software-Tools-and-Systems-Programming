#include <stdio.h>
#include <stdlib.h>

/* 节点结构体 */
typedef struct node {
    int value;
    struct node *next;
} Node;

/* 创建节点: Create and return a new Node with the supplied values. */
Node *create_node(int num, Node *next) {
    Node *new_node = malloc(sizeof(Node));		// 堆上动态分配内存 返回节点指针
    new_node->value = num;
    new_node->next = next;
    return new_node;
}

/* 创建链表 */
int main() {

    Node *front = NULL;					// 初始化头指针指向空 front -> NULL
    front = create_node(3, front);		// front -> Node(3) -> NULL
    front = create_node(2, front);		// front -> Node(2) -> Node(3) -> NULL
    front = create_node(1, front);		// front -> Node(1) -> Node(2) -> Node(3) -> NULL

    Node *curr = front;
    while (curr != NULL) {
        printf("%d\n", curr->value);
        curr = curr->next;
    }
    return 0;
}
