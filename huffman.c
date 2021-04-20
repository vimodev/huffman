#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "node.h"

struct node **create_initial_nodes(FILE *ptr);
unsigned long long int *get_byte_distribution(FILE *ptr);
struct node *huffman_combine(struct node **nodes);
void compress(FILE *ptr, char *path, struct node *tree);
unsigned char buffer_to_byte(unsigned char *buffer);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        perror("Expected 3 arguments.\n");
        exit(EXIT_FAILURE);
    }

    // Open file
    FILE *ptr;
    ptr = fopen(argv[1], "rb");

    // Create initial leaves
    struct node **nodes = create_initial_nodes(ptr);

    // Form the tree
    struct node *root = huffman_combine(nodes);

    // printf("%s\n", encode(root, (unsigned char) 153));

    compress(ptr, argv[2], root);

    // Clean up memory
    for (int i = 0; i < 512; i++) {
        if (nodes[i] != NULL) {
            free(nodes[i]->bytes);
            free(nodes[i]);
        }
    }
    free(nodes);

    fclose(ptr);

    return 0;
}

void compress(FILE *ptr, char *path, struct node *tree) {
    // Create new file
    FILE *new_file;
    new_file = fopen(path, "wb+");
    // Compress
    fseek(ptr, 0, SEEK_SET);
    // Store byte read
    unsigned char read_byte;
    // Store return value of encoding
    unsigned char *encoding;
    // Buffer to write byte by byte
    unsigned char buffer[8] = {0};
    // Where are we now in the buffer
    int buffer_head = 0;
    // Where are we now in the return array
    int encoding_head;
    while (!feof(ptr)) {
        // Read a byte
        fread(&read_byte, sizeof(read_byte), 1, ptr);
        // Encode it
        encoding = encode(tree, read_byte);
        // Move it into the buffer
        encoding_head = 1;
        while (encoding_head != encoding[0]) {
            // Write char into buffer
            buffer[buffer_head++] = encoding[encoding_head++];
            // If buffer is full
            if (buffer_head >= 8) {
                // Create a writable byte
                unsigned char write_byte = buffer_to_byte(buffer);
                buffer_head = 0;
                // and write it to file
                fwrite(&write_byte, 1, 1, new_file);
            }
        }
        free(encoding);
    }
    // If buffer is not empty
    if (buffer_head != 0) {
        // Fill buffer with 0's
        while (buffer_head < 8) {
            buffer[buffer_head++] = '0';
        }
        // And create a byte
        char write_byte = (char) strtol(buffer, NULL, 2);
        buffer_head = 0;
        // To write
        fwrite(&write_byte, 1, 1, new_file);
    }
    fclose(new_file);
}

/**
 * Given file, create initial nodes for huffman tree
 */
struct node **create_initial_nodes(FILE *ptr) {
    // Get the distribution
    unsigned long long int *dist = get_byte_distribution(ptr);
    // Allocate array of pointers to nodes
    // Size 2n for worst case n leaves of binary tree
    struct node **nodes = (struct node**) malloc(512 * sizeof(struct node *));
    // Keep track of head
    int head = 0;
    // Loop over all possible bytes in distribution
    for (int i = 0; i < 256; i++) {
        // If there were any
        if (dist[i] > 0) {
            // Create a node and move the head
            nodes[head] = node_create((unsigned char) i, dist[i]);
            head++;
        }
    }
    free(dist);
    // Set rest to NULL
    for (int i = head; i < 512; i++) {
        nodes[head] = NULL;
    }
    return nodes;
}

/**
 * Count occurence of each byte in the file
 */
unsigned long long int *get_byte_distribution(FILE *ptr) {
    // Reset pointer to start just in case
    fseek(ptr, 0, SEEK_SET);
    // Define the array to contain distribution
    unsigned long long int *dist = (unsigned long long int *) calloc(256, sizeof(unsigned long long int));
    unsigned char buffer;
    // Go over entire file, all bytes
    while (!feof(ptr)) {
        // Increment the distribution based on the byte found
        fread(&buffer, sizeof(buffer), 1, ptr);
        dist[buffer]++;
    }
    return dist;
}

/**
 * Given the array of initial nodes, combine them
 * like the huffman algorithm specifies, and return the root node.
 */
struct node *huffman_combine(struct node **nodes) {
    // Find the head
    int head = 0;
    while (nodes[head] != NULL) {
        head++;
    }
    int index1, index2;
    // Keep combining nodes until break
    while (true) {
        long long int min1 = 100000000000;
        long long int min2 = 100000000001;
        for (int i = 0; i < head; i++) {
            // If null continue
            if (nodes[i] == NULL) continue;
            // If less than 2nd min, replace it
            if (nodes[i]->count < min2) {
                min2 = nodes[i]->count;
                index2 = i;
            }
            // And if also less than 1st min, swap them
            if (min2 < min1) {
                min2 = min1; min1 = nodes[i]->count;
                index2 = index1; index1 = i;
            }
        }
        // If min2 was never found return node 1, it must
        // have been the last node
        if (min2 > 99999999999) return nodes[index1];
        // Otherwise create parent for found nodes
        nodes[head++] = node_create_parent(nodes[index1], nodes[index2]);
        // And remove the found nodes from array
        nodes[index1] = NULL; nodes[index2] = NULL;
    }

    return NULL;
}

unsigned char buffer_to_byte(unsigned char *buffer) {
    unsigned char result = (unsigned char) 0;
    for (int i = 0; i < 8; i++) {
        result |= (unsigned char) 1;
        result << 1;
    }
}