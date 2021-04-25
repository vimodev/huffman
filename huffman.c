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
unsigned char *byte_to_buffer(unsigned char byte);
void make_file_table(FILE *ptr, unsigned char *seq, struct node *node);
unsigned char **read_table(FILE *ptr);
struct node *create_tree_from_table(unsigned char **table);
void add_byte_to_tree(struct node *tree, unsigned char *sequence);
void decompress(FILE *ptr, char *path, struct node *tree);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        perror("Expected 4 arguments.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "-c") == 0) {
        // Open file
        FILE *ptr;
        ptr = fopen(argv[2], "rb");

        // Create initial leaves
        struct node **nodes = create_initial_nodes(ptr);

        // Form the tree
        struct node *root = huffman_combine(nodes);

        compress(ptr, argv[3], root);

        // Clean up memory
        for (int i = 0; i < 512; i++) {
            if (nodes[i] != NULL) {
                free(nodes[i]->bytes);
                free(nodes[i]);
            }
        }
        free(nodes);

        fclose(ptr);
    } else if (strcmp(argv[1], "-d") == 0) {
        // Open file
        FILE *ptr;
        ptr = fopen(argv[2], "rb");
        // Read the table from the file
        unsigned char **table = read_table(ptr);
        // Create the tree from the table
        struct node *root = create_tree_from_table(table);
        decompress(ptr, argv[3], root);
        fclose(ptr);
    }

    return 0;
}

void decompress(FILE *ptr, char *path, struct node *tree) {
    // Create new file
    FILE *new_file;
    new_file = fopen(path, "wb+");
    unsigned char read_byte;
    unsigned char *buffer;
    struct node *n = tree;
    while (!feof(ptr)) {
        fread(&read_byte, sizeof(read_byte), 1, ptr);
        buffer = byte_to_buffer(read_byte); 
        for (int head = 0; head < 8; head++) {
            if (node_is_leaf(n)) {
                fwrite(n->bytes, 1, 1, new_file);
                n = tree;
            }
            if (buffer[head] == 0) n = n->left;
            else if (buffer[head] == 1) n = n->right;
        }
        free(buffer);
    }
    fclose(new_file);
}

void compress(FILE *ptr, char *path, struct node *tree) {
    // Create new file
    FILE *new_file;
    new_file = fopen(path, "wb+");
    // Write huffman encodings
    unsigned char *seq = (unsigned char *) malloc(257 * sizeof(unsigned char));
    seq[0] = 1;
    make_file_table(new_file, seq, tree);
    fseek(new_file, -1, SEEK_CUR);
    unsigned char buf = 255;
    fwrite(&buf, 1, 1, new_file);
    //free(seq);
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
            buffer[buffer_head++] = (unsigned char) 0;
        }
        // And create a byte
        unsigned char write_byte = buffer_to_byte(buffer);
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
        result <<= 1;
        result += (unsigned char) buffer[i];
    }
    return result;
}

unsigned char *byte_to_buffer(unsigned char byte) {
    unsigned char *buffer = calloc(8, sizeof(unsigned char));
    for (int i = 7; i >= 0; i--) {
        buffer[i] = byte % 2;
        byte >>= 1;
    }
    return buffer;
}

void make_file_table(FILE *ptr, unsigned char *seq, struct node *node) {
    if (node_is_leaf(node)) {
        int seq_end = seq[0];
        fwrite(node->bytes, 1, 1, ptr);
        for (int i = 1; i < seq_end; i++) {
            fwrite(&seq[i], 1, 1, ptr);
        }
        unsigned char two = (unsigned char) 2;
        fwrite(&two, 1, 1, ptr);
        return;
    }
    // Recurse left
    seq[seq[0]++] = (unsigned char) 0;
    make_file_table(ptr, seq, node->left);
    // Recurse right
    seq[seq[0] - 1] = (unsigned char) 1;
    make_file_table(ptr, seq, node->right);
    // Move sequence head back
    seq[0]--;
}

unsigned char **read_table(FILE *ptr) {
    // Make a table in memory
    unsigned char **table = (unsigned char **) malloc(256 * sizeof(unsigned char *));
    int table_head = 1;
    unsigned char row_head;
    // Reset pointer to start just in case
    fseek(ptr, 0, SEEK_SET);
    unsigned char buffer;
    bool prev_was_delim = true;
    // Go over entire file, all bytes
    while (!feof(ptr)) {
        // Increment the distribution based on the byte found
        fread(&buffer, sizeof(buffer), 1, ptr);
        // ff denotes end of huffman table
        if (buffer == (unsigned char) 255) {
            table[table_head][1] = row_head;
            table_head++;
            break;
        }
        // Was the previous byte a delimiting byte?
        if (prev_was_delim) {
            // Create a new row for a new byte
            table[table_head] = malloc(257 * sizeof(unsigned char));
            // Denote for which byte it is
            table[table_head][0] = buffer;
            row_head = 2;
            prev_was_delim = false;
        // Is the current byte delimiting
        } else if (buffer == (unsigned char) 2) {
            // assign it as such
            prev_was_delim = true;
            // Finalize the current table row
            table[table_head][1] = row_head;
            table_head++;
        // We are reading encoding
        } else {
            // append it to the table row
            table[table_head][row_head++] = buffer;
        }
    }
    // Put the head at the front of the table
    table[0] = (unsigned char *) malloc(sizeof(unsigned char));
    table[0][0] = table_head;
    return table;
}

struct node *create_tree_from_table(unsigned char **table) {
    struct node *root = node_create(0, 0);
    for (int i = 1; i < table[0][0]; i++) {
        add_byte_to_tree(root, table[i]);
    }
    return root;
}

void add_byte_to_tree(struct node *tree, unsigned char *sequence) {
    struct node *subtree = tree;
    for (int i = 2; i < sequence[1]; i++) {
        if (sequence[i] == 0) {
            if (subtree->left == NULL) subtree->left = node_create(0, 0);
            subtree = subtree->left;
        } else if (sequence[i] == 1) {
            if (subtree->right == NULL) subtree->right = node_create(0, 0);
            subtree = subtree->right;
        } else {
            
        }
    }
    *(subtree->bytes) = sequence[0];
}