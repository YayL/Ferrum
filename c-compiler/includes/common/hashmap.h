#pragma once

#include "common/common.h"

typedef struct HM_pair {
    char is_set;
	char * key;
	void * value;
} HM_Pair;

struct HashMap {
	size_t buckets;
	size_t total;
    struct SparseList * list;
};

struct HashMap * init_hashmap(size_t pow_capacity);
long hashmap_hashcode(struct HashMap * map, const char * key);

void * hashmap_get(struct HashMap * map, const char * key);
char hashmap_has(struct HashMap * map, const char * key);
void hashmap_set(struct HashMap * map, char * key, void* value);
void * hashmap_remove(struct HashMap * map, const char * key);

void hashmap_print(struct HashMap * map);
void hashmap_clear (struct HashMap * map);
void hashmap_free(struct HashMap * map);
