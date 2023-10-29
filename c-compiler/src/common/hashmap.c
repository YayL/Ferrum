#include "common/hashmap.h"
#include "codegen/AST.h"

#include "fmt.h"

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
	HM_Pair * value = sparselist_get(map->list + hashmap_hashcode(map, key), key);
    if (value == NULL)
        return NULL;
    return value->value;
}

char hashmap_has(struct HashMap * map, const char * key) {
	return sparselist_get(map->list + hashmap_hashcode(map, key), key) != NULL;
}

void hashmap_set(struct HashMap * map, char * key, void * value) {
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
