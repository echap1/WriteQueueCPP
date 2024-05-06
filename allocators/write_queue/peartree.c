//
// Created by Rylie Anderson on 3/6/24.
//
#include "peartree.h"

//Convenience integer size constant in bits
#define INTSIZE (int)(sizeof(int) * 8)

//Calculate the block size for a given layer or class
#define block(layerc, layer) (MINIMUM << (layerc - (layer) - 1))

//Locate the memory block associated with an index and class
#define locate(tree, class, index) (tree->base + (index * block(tree->layers, class)))

//Calculate the parent node index of a child node index
#define parent(index) (index / 2)

//Calculate the child node index of a parent node index
#define child(index) (index * 2)

//Retrieve the value of any node for any class or layer, not guaranteed to be one or zero
#define value(tree, class, layer, index) \
    (tree->branches[class][layer][(index) / INTSIZE] & (1 << ((index) % INTSIZE)))

//Set the value of a node at any layer to one
#define set(tree, class, layer, index) \
    assert(index < tree->segments);      \
    tree->branches[class][layer][(index) / INTSIZE] |= (1 << ((index) % INTSIZE))

//Set the value of a node at any layer to zero
#define unset(tree, class, layer, index) \
    tree->branches[class][layer][(index) / INTSIZE] &= ~(1 << ((index) % INTSIZE))

//Retrieve the values of the children of a given node
#define pear(tree, class, layer, index) (\
    (tree->branches[class][layer + 1][child(index) / INTSIZE] & (3 << (child(index) % INTSIZE)))\
    >> (child(index) % INTSIZE)\
)

//Calculate the index of an allocation block
#define ialloc(tree, class, index) (index << (tree->layers - (class) - 1))

//Round-up integer division
#define seg(num, den) ((num / den) + ((num) % (den) ? 1 : 0))

//Calculate number of ints required to store a bitset of a given size
#define pack(l) (seg(l, INTSIZE))

//Calculate the size in ints required to store a layers state
#define sizer(len, layerc, layer) pack(seg(len, block(layerc, layer)))

//Initialize a PearTree
void init(PearTree* tree, void* start, long len) {

    ///Determine parameters of state region

    //Count the number of tree layers required
    int layers = 1;
    for (long con = len; con > MINIMUM; con = (con >> 1) + (con & 1), layers++);
    //Calculate the required capacity of each layer with Gauss's formula and store in sizes space
    long allocs = len / MINIMUM;
    int initial = seg((int)sizeof(char) * allocs, (int)sizeof(int)) * (int)sizeof(int);
    int begin = initial + (int)sizeof(long) * 2 * layers;
    int middle = begin + (int)sizeof(int*) * layers;
    int overhead = middle + (int)sizeof(int*) * ((layers * (layers + 1)) / 2);

    ///Initialize tree pointers

    //Tree pointer
    int*** branches = start + begin;
    //Subtree pointer
    int** head = start + middle;
    //Branch pointer
    int* tail = start + overhead;
    for (int i = 0; i < layers; i++) {
        //Set subtree location
        ((int***)branches)[i] = head;
        for (int j = 0; j <= i; j++) {
            //Set branch location
            head[j] = tail;
            tail += sizer(len, layers, j);
        }
        head += i + 1;
    }

    ///Initialize state values

    //Loop through every class
    long* queue = start + initial;
    long* tails = (void*)queue + (int)sizeof(long) * layers;
    for (int class = 0; class < layers; class++) {
        //Initialize lists to empty
        queue[class] = -1;
        tails[class] = -1;
        int** trunk = ((int***)branches)[class];
        for (int layer = 0; layer <= class; layer++) {
            int* branch = trunk[layer];
            //Set all branch states to zero
            int width = sizer(len, layers, layer);
            for (int k = 0; k < width; branch[k++] = 0);
        }
    }

    //Initialize allocation flags
    char* alloc = start;
    for (int index = 0; index < allocs; index++) {
        alloc[index] = 0;
    }

    //Final tail value becomes allocation base
    tree->base = tail;
    tree->end = start + len;
    tree->branches = branches;
    tree->layers = layers;
    tree->stack = queue;
    tree->tails = tails;
    tree->alloc = alloc;
    tree->len = len;

    //Initialize reachable branch remnants through greedy change-making
    long rem = (int)(tree->end - tree->base);
    tree->segments = rem / MINIMUM;
    if (debug) printf("Segments: %ld\n", rem / MINIMUM);
    long offset = 0;
    if (debug) printf("Class: ");
    for (int class = 0; class < layers; class++) {
        //Determine if class block size will fit
        int size = block(layers, class);
        if (size <= rem) {
            long index = offset / size;
            if (debug) printf("%d - %d - %ld, ", class, block(layers, class), index);
            for (int layer = class; layer >= 0; layer--) {
                //Set tree edges appropriately
                set(tree, class, layer, index);
                index = parent(index);
            }
            rem -= size;
            offset += size;
        }
    }

    if (debug) printf("\n");
}

/**
 * Pops a node from the relevant class stacks
 * @param tree peartree pointer
 * @param class class to pop from
 * @return the index of the first node
 */
long pop(PearTree* tree, int class) {
    if (!SPLICE) {
        long index = tree->stack[class];
        if (tree->alloc[ialloc(tree, class, index)] != ~class) {
            tree->stack[class] = -1;
            return -1;
        }
    }

    //Retrieve the current list head and reset the head to the next node
    long index = tree->stack[class];
    long next = ((SignPost*)locate(tree, class, index))->next;
    tree->stack[class] = next;
    tree->alloc[ialloc(tree, class, index)] = 0;

    //Set tail to none if only node
    if (tree->tails[class] == index) {
        tree->tails[class] = -1;
    }

    return index;
}

/**
 * Pushes a node onto the priority stack
 * @param tree peartree pointer
 * @param class relevant size class
 * @param index index of node
 */
void push(PearTree* tree, int class, long index) {
    //Retrieve the current list head and set its prev to the new node
    long old = tree->stack[class];
    ((SignPost*)locate(tree, class, old))->prev = index;
    tree->alloc[ialloc(tree, class, index)] = ~((char)class);

    //Set new list head and new node's neighbors
    tree->stack[class] = index;
    SignPost* new = ((SignPost*)locate(tree, class, index));
    new->prev = -1;
    new->next = old;

    //Set list tail if only node
    if (tree->tails[class] == -1) {
        tree->tails[class] = index;
    }
}

/**
 * Deletes a node from anywhere in the list if it exists
 * @param tree peartree pointer
 * @param class relevant size class
 * @param index index of node
 */
void delete(PearTree* tree, int class, long index) {
    if (SPLICE) {
        if (tree->alloc[ialloc(tree, class, index)] == ~class) {
            //Calculate addresses of node and neighbors
            SignPost* post = locate(tree, class, index);
            SignPost* next = locate(tree, class, post->next);
            SignPost* prev = locate(tree, class, post->prev);
            tree->alloc[ialloc(tree, class, index)] = 0;

            //Set list heads and tails if appropriate
            if (tree->stack[class] == index) {
                tree->stack[class] = post->next;
            }

            if (tree->tails[class] == index) {
                tree->tails[class] = post->prev;
            }

            //Reset next's attributes if it exists
            if ((void*)next >= tree->base && (void*)next < tree->end) {
                if (tree->alloc[ialloc(tree, class, post->next)] == ~class) {
                    if (next->prev == index) {
                        next->prev = post->prev;
                    }
                }
            }

            //Reset prev's attributes if it exists
            if ((void*)prev >= tree->base && (void*)prev < tree->end) {
                if (tree->alloc[ialloc(tree, class, post->prev)] == ~class) {
                    if (prev->next == index) {
                        prev->next = post->next;
                    }
                }
            }
        }
    }
    else {
        tree->alloc[ialloc(tree, class, index)] = 0;
    }
}

/**
 * Propagates a positive change up a tree
 * @param tree peartree pointer
 * @param class size class
 * @param index node index
 */
void prograte(PearTree* tree, int class, long index) {
    if (debug) printf("Prograting %d %ld\n", class, index);
    //While the node's parent is zero, set it to one and continue propagating up
    for (int layer = class; layer >= 0; layer--) {
        set(tree, class, layer, index);
        index = parent(index);
    }
}

/**
 * Propagates a negative change up a tree
 * @param tree peartree pointer
 * @param class size class
 * @param index node index
 */
void antigrate(PearTree* tree, int class, long index) {
    //Set given node to zero
    unset(tree, class, class, index);
    index = parent(index);

    //While the node and its sibling are both zero, set parent to zero and continue propogating up
    for (int layer = class - 1; layer >= 0 && !pear(tree, class, layer, index); layer--) {
        unset(tree, class, layer, index);
        index = parent(index);
    }
}

/**
 * Merges the children of given node into a higher class
 * @param tree peartree pointer
 * @param class size class
 * @param index node index
 */
void merge(PearTree* tree, int class, long index) {
    if (debug) printf("Merging %d %ld\n", class, index);
    //Antigrate the child nodes in the lower class and propagate the given node and class
    unset(tree, class + 1, class + 1, child(index) + 1);
    antigrate(tree, class + 1, child(index));
    prograte(tree, class, index);
}

/**
 * Splits a given node into a lower class
 * @param tree peartree pointer
 * @param class size class
 * @param index node index
 */
void split(PearTree* tree, int class, long index) {
    if (debug) printf("Splitting %d %ld\n", class, index);
    //Antigrate given node and propagate children in lower class
    delete(tree, class, index);
    antigrate(tree, class, index);
    prograte(tree, class + 1, child(index));
    set(tree, class + 1, class + 1, child(index) + 1);
}

/**
 * Descends the trees to find the optimum unallocated block
 * @param tree peartree pointer
 * @param class size class
 * @return index of allocated block
 */
long descend(PearTree* tree, int class, bool initial) {
    //If a node exists in the stack, use it
    if (tree->stack[class] >= 0 && QUEUE) {
        return pop(tree, class);
    }

    //If the tree is empty, descend and split
    else if (!tree->branches[class][0][0]) {
        //Unsuccessful base case at top class
        if (class == 0) {
            return -1;
        }
        //Descend in the higher class and split the node found
        long parent = descend(tree, class - 1, false);
        if (parent < 0) {
            return -1;
        }
        split(tree, class - 1, parent);
        return child(parent) + 1;
    }

    //If not empty, traverse the tree
    else {
        long index = 0;
        if (initial) {
            for (int layer = 0; layer < class; layer++) {
                index = 2 * index + !value(tree, class, layer + 1, child(index));
            }
        }
        else {
            for (int layer = 0; layer < class; layer++) {
                index = 2 * index + (value(tree, class, layer + 1, child(index) + 1) ? 1 : 0);
            }
        }

        return index;
    }
}

/**
 * Ascends a given class tree to return a previously allocated block
 * @param tree peartree pointer
 * @param class size class
 * @param index node index
 */
void ascend(PearTree* tree, int class, long index, bool initial) {
    //If not already in the highest class and the sibling is one, merge and ascend
    if (class > 0 && pear(tree, class, class - 1, parent(index)) == 3) {
        //Delete merged nodes from the list
        delete(tree, class, child(parent(index)));
        delete(tree, class, child(parent(index)) + 1);
        merge(tree, class - 1, parent(index));
        ascend(tree, class - 1, parent(index), false);
    }

    //Otherwise, simply push the node into the stack
    else if (initial && QUEUE) {
        push(tree, class, index);
    }
}

void* take(PearTree* tree, long size) {
    //If the request isn't satisfiable, return null
    if (size > block(tree->layers, 0)) {
        return NULL;
    }

    //Determine the size class
    int class;
    for (class = tree->layers - 1; size > block(tree->layers, class); class--);

    //Descend and validate the index
    long index = descend(tree, class, true);
    if (index < 0) {
        return NULL;
    }

    //Antigrate the given index and allocate
    antigrate(tree, class, index);
    tree->alloc[ialloc(tree, class, index)] = (char)(class + 1);
    return locate(tree, class, index);
}

void give(PearTree* tree, void* pointer) {
    //Guard against freeing null memory
    if (pointer == NULL) {
        return;
    }

    //Calculate raw index at the lowest level
    long rindex = (pointer - tree->base) / MINIMUM;

    //If block was previously allocated
    int class = tree->alloc[rindex] - 1;
    if (class > 0) {
        //Calculate true classed index, deallocate, prograte change up tree, and ascend
        long index = (pointer - tree->base) / block(tree->layers, class);
        tree->alloc[ialloc(tree, class, index)] = 0;
        prograte(tree, class, index);
        ascend(tree, class, index, true);
    }
}

/**
 * Prints the current state of the tree
 * @param tree peartree
 */
void display(PearTree* tree, bool verbose) {
    printf("\nTree State:\nAvailability:\n");
    for (int class = 0; class < tree->layers; class++) {
        printf("Class %d (%d bytes): \t", class, MINIMUM * (1 << (tree->layers - class - 1)));
        if (verbose) {
            for (int layer = 0; layer < class; layer++) {
                for (int index = 0; index < seg(seg(tree->len, block(tree->layers, layer)), INTSIZE); index++) {
                    for (int j = 0; j < seg(tree->len, block(tree->layers, layer)) - index * INTSIZE && j < INTSIZE; j++) {
                        printf("%d", (tree->branches[class][layer][index] & 1 << j) ? 1 : 0);
                    }
                }
                printf("\n");
                if (layer < class) {
                    printf("\t\t\t");
                }
            }
        }
        for (int index = 0; index < sizer(tree->len, tree->layers, class); index++) {
            for (int j = 0; j < (tree->len / block(tree->layers, class)) - index * INTSIZE && j < INTSIZE; j++) {
                printf("%d", (tree->branches[class][class][index] & 1 << j) ? 1 : 0);
            }
            printf(" ");
        }
        printf("\n");
    }
    printf("\nAllocation:\t");

    int count = 0;

    for (int i = 0; i < tree->segments; i++) {
        if (i % INTSIZE == 0 && i != 0) {
            printf(" ");
        }
        if (tree->alloc[i] <= 0) {
            printf("-");
        }
        else {
            printf("%d", tree->alloc[i] - 1);
        }
    }

    printf("\nQueueing:\t");

    count = 0;

    for (int i = 0; i < tree->segments; i++) {
        if (i % INTSIZE == 0 && i != 0) {
            printf(" ");
        }
        if (tree->alloc[i] >= 0) {
            printf("-");
        }
        else {
            printf("%d", ~tree->alloc[i]);
        }
    }

    printf(" %d\n\n", count);

    printf("Stacks:\n");
    for (int class = 0; class < tree->layers; class++) {
        if (tree->stack[class] >= 0) {
            printf("Class %d: ", class);
            long index = tree->stack[class];
            while (index >= 0) {
                SignPost* post = ((SignPost*)locate(tree, class, index));
                printf("%ld (%ld, %ld), ", index, post->prev, post->next);
                index = post->next;
            }
            printf("\n");
        }
    }
    printf("\n");
}