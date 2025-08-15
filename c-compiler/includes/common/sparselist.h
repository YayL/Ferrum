#pragma once

#include "common/hashmap.h"

// TODO:
// FIx sparselist...
// There is no need for them to be sparse, it is a lot better to when an element is removed
// just fill that space with the last element in the list?? I have no need of keeping some
// order so why not just fill it with the last element

struct hm_pair {
	unsigned int key;
	unsigned int value;
    char is_set;
};

struct SparseList {
    struct hm_pair * buf;
    unsigned int size;
    unsigned int capacity;
    char is_sparse; // if not contiguous memory, meaning there are empty entries
};

struct SparseList init_sparselist();

void sparselist_set(struct hm_pair * item, char set, unsigned int key, unsigned int value);
struct hm_pair * sparselist_get(const struct SparseList list, unsigned int key);

void sparselist_push(struct SparseList * list, unsigned int key, unsigned int value);
void sparselist_remove(struct SparseList * list, int index);
void sparselist_balance(struct SparseList * list);

void sparselist_combine(struct SparseList * dest, struct SparseList * src);
