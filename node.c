#include "node.h"

/**
 * Given the huffman tree, encode the byte
 */
void encode(struct node *root, unsigned char byte, char *encoding) {
    struct node *n = root;
    int head = 1;
    while (n->left != NULL && n->right != NULL) {
        if (n->left->contains[byte]) {
            n = n->left;
            encoding[head++] = (unsigned char) 0;
        } else {
            n = n->right;
            encoding[head++] = (unsigned char) 1;
        }
    }
    encoding[0] = (unsigned char) head;
}

/**
 * Given child nodes, create a parent node
 */
struct node *node_create_parent(struct node *left, struct node *right) {
    // Allocate new node
    struct node *new_node = (struct node *) malloc(sizeof(struct node));
    // Allocate new byte array
    new_node->nr_of_bytes = left->nr_of_bytes + right->nr_of_bytes;
    new_node->bytes = (unsigned char *) malloc(new_node->nr_of_bytes * sizeof(unsigned char));
    // Copy the bytes correspondingly
    new_node->contains = (bool *) calloc(256, sizeof(bool));
    for (int i = 0; i < 256; i++) new_node->contains[i] = left->contains[i] || right->contains[i];
    // Assign children
    new_node->left = left;
    new_node->right = right;
    // Sum the count
    new_node->count = left->count + right->count;
    return new_node;
}

/**
 * Create a new node
 */ 
struct node *node_create(unsigned char byte, unsigned long long int count) {
    struct node *new_node = (struct node *) malloc(sizeof(struct node));
    new_node->bytes = (unsigned char *) malloc(sizeof(unsigned char));
    new_node->contains = (bool *) calloc(256, sizeof(bool));
    new_node->contains[byte] = true;
    new_node->bytes[0] = byte;
    new_node->count = count;
    new_node->left = NULL;
    new_node->right = NULL;
    new_node->nr_of_bytes = 1;
    return new_node;
}

/**
 * Return whether the given node is a leaf
 */
bool node_is_leaf(struct node *x) {
    return x->left == NULL && x->right == NULL;
}

void node_free(struct node *subtree) {
    free(subtree->bytes);
    if (!node_is_leaf(subtree)) {
        node_free(subtree->left);
        node_free(subtree->right);
    }
    free(subtree);
}