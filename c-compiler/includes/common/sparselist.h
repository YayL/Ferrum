#pragma once

#include "common/common.h"

struct SparseList {
    void ** buf;
    size_t item_size;
    unsigned int size;
    unsigned int capacity;
    char is_sparse; // if not contiguous memory, meaning there are empty entries
};

struct SparseList * init_sparselist(size_t item_size);

void sparselist_push(struct SparseList * list, void * item);
void sparselist_remove(struct SparseList * list, int index);

void sparselist_balance(struct SparseList * list);

