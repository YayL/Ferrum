#pragma once

#include "common/hashmap.h"

struct SparseList {
    HM_Pair * buf;
    unsigned int size;
    unsigned int capacity;
    char is_sparse; // if not contiguous memory, meaning there are empty entries
};

struct SparseList * init_sparselist();

void sparselist_set(HM_Pair * item, char set, char * key, void * value);
void sparselist_push(struct SparseList * list, char * key, void * value);
void sparselist_remove(struct SparseList * list, int index);
HM_Pair * sparselist_get(const struct SparseList * list, const char * key);
void sparselist_balance(struct SparseList * list);
