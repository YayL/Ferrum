#include "common/sparselist.h"

struct SparseList * init_sparselist(size_t item_size) {
    struct SparseList * list = calloc(1, sizeof(struct SparseList));
    list->item_size = item_size;

    return list;
}

void sparselist_push(struct SparseList * list, void * item) {
    if (list->buf == NULL) {
        list->buf = calloc(1, list->item_size);
        list->capacity = 1;
    } else if (list->size == list->capacity) {
        list->capacity *= 2;
        list->buf = realloc(list->buf, list->capacity * list->item_size);
        list->is_sparse = 0;
        memset(list->buf + list->size + 1, 0, list->size - 1);
    }

    if (!list->is_sparse) {
        list->buf[list->size++] = item;
        return;
    }

    const int cap = list->capacity;

    // insert at first empty entry
    for (int i = 0; i < cap; ++i) {
        if (!list->buf[i]) {
            list->buf[i] = item;
            list->is_sparse = (i != ++list->size);
            break;
        }
    }
}

void sparselist_remove(struct SparseList * list, int index) {
    const int cap = list->capacity, size = list->size;
    int count = 0;
    index = (size + (index % size)) % size;
    list->size -= 1;

    if (!list->is_sparse) {
        list->buf[index] = NULL;
        list->is_sparse = list->size != index;
        return;
    }

    for (int i = 0; i < cap; ++i) {
        if (list->buf[i] && count++ == index) {
            list->buf[i] = NULL;
            break;
        }
    }
}

void sparselist_balance(struct SparseList * list) {
    void ** buffer = calloc(list->capacity, list->item_size);
    const int size = list->size;

    for (int i = 0, index = 0; index < size; ++i) {
        if (list->buf[i]) {
            buffer[index++] = list->buf[i];
        }
    }

    list->is_sparse = 0;

    free(list->buf);
    list->buf = buffer;
}
