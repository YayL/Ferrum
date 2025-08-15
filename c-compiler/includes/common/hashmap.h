#pragma once

#include "common/common.h"
#include "common/sparselist.h"

typedef struct hashmap {
	struct SparseList * lists;
	size_t buckets;
	size_t total;
} HashMap;

HashMap hashmap_init(size_t pow_capacity);
unsigned int hashmap_hashcode(struct hashmap map, unsigned int key);

char hashmap_has(HashMap map, unsigned int key);
void hashmap_set(HashMap * map, unsigned int key, unsigned int value);

unsigned int hashmap_get(HashMap map, unsigned int key);
unsigned int hashmap_remove(HashMap * map, unsigned int key);

void hashmap_combine(HashMap * dest, HashMap src);

void hashmap_print(HashMap map);
void hashmap_clear (HashMap * map);
void hashmap_free(HashMap * map);
