#include <stdio.h>
#include <stdlib.h>

/* 节点结构体 */
typedef struct node {
    int value;
    struct node *next;
} Node;

/* 创建节点: Create and return a new Node with the supplied values. */
Node *create_node(int num, Node *next) {
    Node *new_node = malloc(sizeof(Node));    // 堆上动态分配内存 返回节点指针
    new_node->value = num;
    new_node->next = next;
    return new_node;
}

/* 插入节点(错误反例）: Insert a new node with the value num into this position of list front. */
void insert(int num, Node *front, int position) {
    Node *curr = front;		// 头指针的副本 传递值(value)
    for (int i = 0; i < position - 1; i++) {
        curr = curr->next;
    }
    printf("Currently at: %d\n", curr->value);

    Node *new_node = create_node(num, curr->next);
    curr->next = new_node;
}

/* 创建链表 */
int main() {
    Node *front = NULL;					// 初始化头指针指向空 front -> NULL
    front = create_node(4, front);		// front -> 节点(4) -> NULL
    front = create_node(3, front);		// front -> 节点(3) -> 节点(4) -> NULL
    front = create_node(1, front);		// front -> 节点(1) -> 节点(3) -> 节点(4) -> NULL

    insert(2, front, 1);				// front -> 节点(1) -> 节点(2) -> 节点(3) -> 节点(4) -> NULL
										// 链表节点索引 index start from 0
    Node *curr = front;
    while (curr != NULL) {
        printf("%d\n", curr->value);
        curr = curr->next;
    }
    return 0;
}
