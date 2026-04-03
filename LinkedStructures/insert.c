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

/* Insert a new node with the value num into this position of list front. */
void insert(int num, Node *front, int position) {
    Node *curr = front;
    for (int i = 0; i < position - 1; i++) {
        curr = curr->next;
    }
    printf("Currently at: %d\n", curr->value);

    Node *new_node = create_node(num, curr->next);
    curr->next = new_node;
}

int main() {
    Node *front = NULL;
    front = create_node(4, front);
    front = create_node(3, front);
    front = create_node(1, front);

    insert(2, front, 1);

    Node *curr = front;
    while (curr != NULL) {
        printf("%d\n", curr->value);
        curr = curr->next;
    }
    return 0;
}
