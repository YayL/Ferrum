#include "common/hashmap.h"

#include "fmt.h"

struct SparseList {
    HM_Pair * buf;
    unsigned int size;
    unsigned int capacity;
    char is_sparse; // if not contiguous memory, meaning there are empty entries
};

struct SparseList * init_sparselist() {
    struct SparseList * list = calloc(1, sizeof(struct SparseList));

    return list;
}

void sparselist_set(HM_Pair * item, char set, char * key, void * value) {
    item->is_set = set;
    item->key = key;
    item->value = value;
}

void sparselist_push(struct SparseList * list, char * key, void * value) {
    if (list->buf == NULL) {
        list->buf = calloc(1, sizeof(HM_Pair));
        list->capacity = 1;
    } else if (list->size == list->capacity) {
        list->capacity *= 2;
        list->buf = realloc(list->buf, list->capacity * sizeof(HM_Pair));
        list->is_sparse = 0;
        memset(list->buf + list->size + 1, 0, list->size - 1);
    }

    if (!list->is_sparse) {
        sparselist_set(list->buf + list->size++, 1, key, value);
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

HM_Pair * sparselist_get(const struct SparseList * list, const char * key) {
    const int size = list->size;

    for (int i = 0, index = 0; index < size; ++i) {
        if (list->buf[i].is_set) {
            index += 1;
            if (!strcmp(list->buf[i].key, key)) {
                return list->buf + i;
            }
        }
    }

    return NULL;
}

void sparselist_balance(struct SparseList * list) {
    HM_Pair * buffer = calloc(list->capacity, sizeof(HM_Pair));
    const int size = list->size;

    for (int i = 0, index = 0; index < size; ++i) {
        if (list->buf[i].is_set) {
            buffer[index++] = list->buf[i];
        }
    }

    list->is_sparse = 0;

    free(list->buf);
    list->buf = buffer;
}

struct HashMap * init_hashmap(size_t pow_capacity) {

	struct HashMap * map = malloc(sizeof(struct HashMap));
	
	map->buckets = (1 << pow_capacity) - 1;
	map->total = 0;

	map->list = calloc(map->buckets + 1, sizeof(struct SparseList));

	return map;
}

long hashmap_hashcode(struct HashMap * map, const char * key) {

	const int p = (2 << 7) - 1;
	const int m = (2 << 24) - 1;
	
	long long hash_value = 0;
	long long p_pow = 31;

	for (int i = 0; key[i]; ++i) {
		hash_value = (((hash_value + (key[i] - ' ' + 1)) * p_pow) & m);
		p_pow = (p_pow * p) & m;
	}
	
	return hash_value & map->buckets;
}

void * hashmap_get(struct HashMap * map, const char * key) {
	return sparselist_get(map->list + hashmap_hashcode(map, key), key)->value;
}

char hashmap_has(struct HashMap * map, const char * key) {
	return sparselist_get(map->list + hashmap_hashcode(map, key), key) != NULL;
}

void hashmap_set(struct HashMap * map, char * key, void* value) {
    struct SparseList * list = map->list + hashmap_hashcode(map, key);
    HM_Pair * pair = sparselist_get(list, key); 
    
    if (pair != NULL) {
        pair->value = value;
        return;
    }

    sparselist_push(list, key, value);
    map->total += 1;
}

void * hashmap_remove(struct HashMap * map, const char * key) {
	HM_Pair * current = sparselist_get(map->list + hashmap_hashcode(map, key), key);
	
	if (current == NULL) {
		println("[HashMap]: Key '{s}' was not found", key);
		exit(1);
	}

    current->is_set = 0;
    map->total -= 1;

	return current->value;
}

void hashmap_print(struct HashMap * map) {
	HM_Pair * bucket, * current;
	println("[Buckets: {lu}, Total: {lu}]:", map->buckets, map->total);
	for (int i = 0; i <= map->buckets; ++i) {
        struct SparseList list = map->list[i];
		bucket = list.buf;
        print("{i}({i}):", i + 1, list.size);
		for (int j = 0; j < list.size;) {
            current = bucket + j;
            if (!current->is_set)
                continue;
			print(" {s},", current->key);
            ++j;
		}
        println("");
	}
}

void hashmap_clear (struct HashMap * map) {
	for (int i = 0; i < map->buckets; ++i) {
        struct SparseList list = map->list[i];
        free(list.buf);
        list.buf = NULL;
        list.size = 0;
        list.capacity = 0;
        list.is_sparse = 0;
	}
}

void hashmap_free(struct HashMap * map) {	
	for (int i = 0; i < map->buckets; ++i) {
        free(map->list[i].buf);
    }
	
	free(map->list);
	free(map);
}
