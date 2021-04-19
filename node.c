#include "node.h"

/**
 * Does the node represent the given byte?
 */
bool bytes_contain(struct node *n, char byte) {
    for (int i = 0; i < n->nr_of_bytes; i++) {
        if (n->bytes[i] == byte) return true;
    }
    return false;
}

/**
 * Given the huffman tree, encode the byte
 */
char* encode(struct node *root, char byte) {
    struct node *n = root;
    char *encoding = malloc(9 * sizeof(char));
    strcpy(encoding, "");
    int depth = 0;
    while (!node_is_leaf(n)) {
        depth++;
        if (bytes_contain(n->left, byte)) {
            n = n->left;
            strcat(encoding, "0");
        } else if (bytes_contain(n->right, byte)) {
            n = n->right;
            strcat(encoding, "1");
        } else {
            break;
        }
    }
    return encoding;
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
    for (int i = 0; i < left->nr_of_bytes; i++) new_node->bytes[i] = left->bytes[i];
    for (int i = left->nr_of_bytes; i < new_node->nr_of_bytes; i++) new_node->bytes[i] = right->bytes[i - left->nr_of_bytes];
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