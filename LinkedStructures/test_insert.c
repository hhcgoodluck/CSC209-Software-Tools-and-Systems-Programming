#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int value;
    struct node *next;
} Node;

/* Create and return a new Node with the supplied values. */
Node *create_node(int num, Node *next) {
    Node *new_node = malloc(sizeof(Node));
    new_node->value = num;
    new_node->next = next;
    return new_node;
}

/* Insert a new node with the value num into this position of list front. 
   Return 0 on success and -1 on failure. */
int insert(int num, Node **front_ptr, int position) {
    Node *curr = *front_ptr;

    if (position == 0) {
        *front_ptr = create_node(num, *front_ptr);
        return 0;
    }

    for (int i = 0; i < position - 1 && curr != NULL; i++) {
        curr = curr->next;
    }
    if (curr == NULL) {
        return -1;
    }
    Node *new_node = create_node(num, curr->next);
    curr->next = new_node;

    return 0;
}

int main() {
    Node *front = NULL;
    front = create_node(4, front);
    front = create_node(3, front);
    front = create_node(1, front);

    insert(2, &front, 1);
    insert(5, &front, 4);        // insert at end
    insert(9000, &front, 9000);  // insert at invalid index
    insert(0, &front, 0);         // insert at the front

    Node *curr = front;
    while (curr != NULL) {
        printf("%d\n", curr->value);
        curr = curr->next;
    }
    return 0;
}
