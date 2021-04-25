#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct node {
    unsigned char *bytes;
    unsigned int nr_of_bytes;
    unsigned long long int count;
    struct node *left;
    struct node *right;
};

void encode(struct node *root, unsigned char byte, char *encoding);
struct node *node_create_parent(struct node *left, struct node *right);
struct node *node_create(unsigned char byte, unsigned long long int count);
bool node_is_leaf(struct node *x);
void node_free(struct node *subtree);