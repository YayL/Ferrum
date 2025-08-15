#include "common/sparselist.h"
#include "common/hashmap.h"

struct SparseList init_sparselist() {
    return (struct SparseList) {0};
}

void sparselist_set(struct hm_pair * item, char set, unsigned int key, unsigned int value) {
    item->is_set = set;
    item->key = key;
    item->value = value;
}

struct hm_pair * sparselist_get(const struct SparseList list, unsigned int key) {
    for (int i = 0, index = 0; index < list.size; ++i) {
        if (list.buf[i].is_set) {
            index += 1;
            if (list.buf[i].key == key) {
                return &list.buf[i];
            }
        }
    }

    return NULL;
}

void sparselist_push(struct SparseList * list, unsigned int key, unsigned int value) {
    if (list->buf == NULL) {
        list->buf = calloc(1, sizeof(struct hm_pair));
        list->capacity = 1;
    } else if (list->size == list->capacity) {
        list->capacity *= 2;
        list->buf = realloc(list->buf, list->capacity * sizeof(struct hm_pair));
        list->is_sparse = 0;
        memset(&list->buf[list->size + 1], 0, (list->capacity - list->size) * sizeof(struct hm_pair));
    }

    if (!list->is_sparse) {
        sparselist_set(&list->buf[list->size++], 1, key, value);
        return;
    }

    const int cap = list->capacity;

    // insert at first empty entry
    for (int i = 0; i < cap; ++i) {
        if (!list->buf[i].is_set) {
            sparselist_set(list->buf + i, 1, key, value);
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
        list->buf[index].is_set = 0;
        list->is_sparse = list->size != index;
        return;
    }

    for (int i = 0; count < size; ++i) {
        if (list->buf[i].is_set && count++ == index) {
            list->buf[i].is_set = 0;
            break;
        }
    }
}

void sparselist_balance(struct SparseList * list) {
    /* HM_Pair * buffer = calloc(list->capacity, sizeof(HM_Pair)); */
    const int size = list->size;
    int index = 0;

    for (int i = 0; index < size; ++i) {
        if (list->buf[i].is_set) {
            /* buffer[index++] = list->buf[i]; */
            list->buf[index++] = list->buf[i];
        }
    }

    list->is_sparse = 0;
    list->size = index + 1;

    /* free(list->buf); */
    /* list->buf = buffer; */
}

void sparselist_combine(struct SparseList * dest, struct SparseList * src) {
    if (src->size == 0) {
        return;
    }

    if (dest->is_sparse) {
        sparselist_balance(dest);
    }
    if (src->is_sparse) {
        sparselist_balance(src);
    }

    const int size = dest->size + src->size;

    if (dest->capacity < size) {
        dest->capacity = size;
        dest->buf = realloc(dest->buf, dest->capacity * sizeof(struct hm_pair));
    }

    for (int i = dest->size; i < size; ++i) {
        dest->buf[i] = src->buf[i - dest->size];
    }
    dest->size = size;
}
