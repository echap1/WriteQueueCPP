//
// Created by Rylie Anderson on 3/6/24.
//

#ifndef BINARY_BUDDY_PEARTREE_H
#define BINARY_BUDDY_PEARTREE_H

#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#define debug false

//Smallest block size
//Must be at least as large as than two longs
#define MINIMUM 16

#define QUEUE true
#define SPLICE false

/**
 * Structure for distributed linked-indexed list implementation of the write stacks
 */
typedef struct SignPostStruct {
    //Next higher node in the stack
    long prev;

    //Next lower node in the stack
    long next;
} SignPost;

/**
 *
 */
typedef struct PearTreeStruct {
    void* base;
    void* end;
    int*** branches;
    long* stack;
    long* tails;
    char* alloc;
    int layers;
    long len;
    long segments;
} PearTree;

/**
 * Initializes a peartree
 * @param tree tree pointer
 * @param start pointer to the beginning of the memory block
 * @param len bytes in the memory block
 */
void init(PearTree* tree, void* start, long len);

/**
 * Take a memory block of a given size from the peartrees memory
 * @param tree peartree
 * @param size size in bytes
 * @return pointer to the allocated chunk
 */
void* take(PearTree* tree, long size);

/**
 * Give a previously allocated chunk of memory back to the tree
 * @param tree peartree
 * @param pointer to deallocate
 */
void give(PearTree* tree, void* pointer);

/**
 * Prints the present state of the tree
 * @param tree peartree
 * @param verbose whether to print full trees
 */
void display(PearTree* tree, bool verbose);

#endif //BINARY_BUDDY_PEARTREE_H
