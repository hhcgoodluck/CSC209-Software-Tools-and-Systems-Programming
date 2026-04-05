#include <stdio.h>
#include <stdlib.h>

/* 节点结构体 */
typedef struct node {
    int value;
    struct node *next;
} Node;

/* 创建节点: Create and return a new Node with the supplied values. */
Node *create_node(int num, Node *next) {
    Node *new_node = malloc(sizeof(Node));
    new_node->value = num;
    new_node->next = next;
    return new_node;
}

/* 插入节点: Insert a new node with the value num into this position of list front.
   Return 0 on success and -1 on failure. */
int insert(int num, Node **front_ptr, int position) {

	// Node **front_ptr 二重指针 front_ptr -> ptr_var -> front
	// 传递地址(address / reference)
    Node *curr = *front_ptr;	// 解引用(dereference) 普通指针

	// 边界处理: 插入位置为 index 0
    if (position == 0) {
        *front_ptr = create_node(num, *front_ptr);
        return 0;
    }
	// 链表索引范围 (0, len) or (0, len-1]
    for (int i = 0; i < position - 1 && curr != NULL; i++) {
        curr = curr->next;
    }
	// 边界处理: 超出索引范围 遍历到链表末尾 NULL
    if (curr == NULL) {
        return -1;
    }
    Node *new_node = create_node(num, curr->next);
    curr->next = new_node;

    return 0;
}

int main() {
    Node *front = NULL;					// 初始化头指针指向空 front -> NULL
    front = create_node(4, front);		// front -> 节点(4) -> NULL
    front = create_node(3, front);		// front -> 节点(3) -> 节点(4) -> NULL
    front = create_node(1, front);		// front -> 节点(1) -> 节点(3) -> 节点(4) -> NULL

	// 插入位置 index 1
    insert(2, &front, 1);			// front -> 节点(1) -> Node(2) -> 节点(3) -> 节点(4) -> NULL
	// 插入末尾: insert at end
    insert(5, &front, 4);         	// front -> 节点(1) -> Node(2) -> 节点(3) -> 节点(4) -> 节点(5) -> NULL
	// 非法插入: insert at invalid index
    insert(9000, &front, 9000);		// return -1
	// 插入开头: insert at the front
    insert(0, &front, 0);         	// front -> 节点(0) -> 节点(1) -> Node(2) -> 节点(3) -> 节点(4) -> 节点(5) -> NULL

    Node *curr = front;
    while (curr != NULL) {
        printf("%d\n", curr->value);
        curr = curr->next;
    }
    return 0;
}
