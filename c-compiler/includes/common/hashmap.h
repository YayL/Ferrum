#pragma once

#include "common/common.h"

typedef struct HM_pair {
	char * key;
	void * value;
	struct HM_pair * next;
} HM_Pair;

struct HashMap {
	HM_Pair ** bucket_list;
	size_t capacity;
	size_t buckets;
	size_t total;
};

struct HashMap * new_HashMap(size_t pow_capacity);
long HM_HashCode(struct HashMap * map, const char * key);

void * HM_get(struct HashMap * map, const char * key);
long HM_has(struct HashMap * map, const char * key);
void HM_set(struct HashMap * map, char * key, void* value);
void * HM_remove(struct HashMap * map, const char * key);

void HM_print(struct HashMap * map);
void HM_clear (struct HashMap * map);
void HM_free(struct HashMap * map);
