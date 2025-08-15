#include "common/hashmap.h"

#include "common/math.h"
#include "common/sparselist.h"
#include "fmt.h"
#include <stdlib.h>

#define KNUTH_CONST ((unsigned int) 2654435769)

HashMap hashmap_init(size_t pow_capacity) {
	size_t buckets = (1 << pow_capacity); 
	return (HashMap) {
		.buckets = buckets,
		.total = 0,
		.lists = calloc(buckets, sizeof(struct SparseList)),
	};
}

unsigned int hashmap_hashcode(struct hashmap map, unsigned int key) {	
	return ((key * KNUTH_CONST) >> (32 - log2i(key))) & (map.buckets - 1);
}

unsigned int hashmap_get(struct hashmap map, unsigned int key) {
	struct hm_pair * value = sparselist_get(map.lists[hashmap_hashcode(map, key)], key);
    if (value == NULL) {
        return -1;
	}
    return value->value;
}

char hashmap_has(struct hashmap map, unsigned int key) {
	return sparselist_get(map.lists[hashmap_hashcode(map, key)], key) != NULL;
}

void hashmap_set(struct hashmap * map, unsigned int key, unsigned int value) {
    struct SparseList * sparselist = &map->lists[hashmap_hashcode(*map, key)];
    struct hm_pair * pair = sparselist_get(*sparselist, key);
    
    if (pair != NULL) {
        pair->value = value;
        return;
    }

    sparselist_push(sparselist, key, value);
    map->total += 1;
}

void hashmap_combine(struct hashmap * dest, struct hashmap src) {
    ASSERT(dest->buckets == src.buckets, "hashmap_combine must have two hashmaps of equal bucket count");
    
    for (int i = 0; i < dest->buckets; ++i) {
        sparselist_combine(dest->lists + i, src.lists + i);
    }

	dest->total += src.total;
}

unsigned int hashmap_remove(struct hashmap * map, unsigned int key) {
	struct hm_pair * current = sparselist_get(map->lists[hashmap_hashcode(*map, key)], key);
	
	if (current == NULL) {
		println("[HashMap]: Key '{s}' was not found", key);
		exit(1);
	}

    current->is_set = 0;
    map->total -= 1;

	return current->value;
}

void hashmap_print(struct hashmap map) {
	struct hm_pair * bucket, * current;
	println("[Buckets: {lu}, Total: {lu}]:", map.buckets, map.total);
	for (int i = 0; i <= map.buckets; ++i) {
        struct SparseList list = map.lists[i];
		bucket = list.buf;
        print("{i}({i}):", i + 1, list.size);
		for (int j = 0; j < list.size;) {
            current = bucket + j;
            if (!current->is_set)
                continue;
			print(" {u},", current->key);
            ++j;
		}
        println("");
	}
}

void hashmap_clear(struct hashmap * map) {
	for (int i = 0; i < map->buckets; ++i) {
        struct SparseList list = map->lists[i];
        free(list.buf);
        list.buf = NULL;
        list.size = 0;
        list.capacity = 0;
        list.is_sparse = 0;
	}
}

void hashmap_free(struct hashmap * map) {	
	for (int i = 0; i < map->buckets; ++i) {
        free(map->lists[i].buf);
    }
	
	free(map->lists);
	free(map);
}
