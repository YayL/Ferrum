#include "tables/interner.h"

#include "common/arena.h"
#include "common/string.h"
#include <stdlib.h>
#include <string.h>

struct interner_hashmap interner_hashmap_init(size_t pow_capacity);
unsigned int interner_hashmap_set(struct interner_hashmap * map, String key, unsigned int value);
unsigned int interner_hashmap_get(struct interner_hashmap map, String key);

Interner interner;

/* ================================ */
/* ====== INTERNER FUNCTIONS ====== */
/* ================================ */

void interner_init() {
	interner = (Interner) {
		.entries = arena_init(sizeof(struct interner_entry)),
		.map = interner_hashmap_init(8),
	};
}

struct interner_entry interner_entry_init(unsigned int ID, String str) {
	return (struct interner_entry) {
		.ID = ID,
		.str = str,
	};
}

unsigned int interner_intern(String string) {
	unsigned int ID = interner_hashmap_get(interner.map, string);

	if (ID != -1) {
		return ID;
	}

	ID = interner.entries.size;
	ARENA_APPEND(&interner.entries, interner_entry_init(ID, string));
	interner_hashmap_set(&interner.map, string, ID);

	return ID;
}

unsigned int interner_lookup_id(const char * str) {
	return INVALID_INTERN_ID;
}

String interner_lookup_str(unsigned int ID);

/* ================================== */
/* ====== SPARSELIST FUNCTIONS ====== */
/* ================================== */

void interner_sparselist_set(struct interner_hm_pair * item, char set, const char * key, unsigned int value) {
	item->is_set = set;
	item->key = key;
	item->value = value;
}

struct interner_hm_pair * interner_sparselist_get(const struct interner_sparselist list, const char * key) {
	for (size_t i = 0, index = 0; index < list.size; ++i) {
		if (!list.buf[i].is_set) {
			continue;
		}

		index += 1;
		if (!strcmp(list.buf[i].key, key)) {
			return &list.buf[i];
		}
	}

	return NULL;
}

void interner_sparselist_push(struct interner_sparselist * sparselist, const char * key, unsigned int value) {
	if (sparselist->buf == NULL) {
		sparselist->buf = calloc(1, sizeof(struct interner_hm_pair));
		sparselist->capacity = 1;
	} else if (sparselist->size == sparselist->capacity) {
		sparselist->capacity *= 2;
		sparselist->buf = realloc(sparselist->buf, sparselist->capacity * sizeof(struct interner_hm_pair));
		sparselist->is_sparse = 0;
		memset(&sparselist->buf[sparselist->size + 1], 0, (sparselist->capacity - sparselist->size) * sizeof(struct interner_hm_pair));
	}

	if (!sparselist->is_sparse) {
		interner_sparselist_set(&sparselist->buf[sparselist->size++], 1, key, value);
	}
}

/* =============================== */
/* ====== HASHMAP FUNCTIONS ====== */
/* =============================== */

struct interner_sparselist interner_sparselist_init();

struct interner_hashmap interner_hashmap_init(size_t pow_capacity) {
	size_t bucket_count = 1 << pow_capacity;
	return (struct interner_hashmap) {
		.lists = calloc(bucket_count, sizeof(struct interner_sparselist)),
		.bucket_count = bucket_count,
		.total = 0,
	};
}

// FNV-1a hash algorithm
unsigned int interner_hashmap_hashcode(struct interner_hashmap map, String key) {
	unsigned int hash = 2166136261;
	for (size_t i = 0; i < key.length; ++i) {
		hash ^= (unsigned char) (key._ptr[i]);
		hash *= 16777619;
	}
	return hash;
}

unsigned int interner_hashmap_get(struct interner_hashmap map, String key) {
	struct interner_hm_pair * pair = interner_sparselist_get(map.lists[interner_hashmap_hashcode(map, key)], key._ptr);
	if (pair == NULL) {
		return -1;
	}

	return pair->value;
}

char interner_hashmap_has(struct interner_hashmap map, String key) {
	return interner_sparselist_get(map.lists[interner_hashmap_hashcode(map, key)], key._ptr) != NULL;
}

unsigned int interner_hashmap_set(struct interner_hashmap * map, String key, unsigned int value) {
	struct interner_sparselist * sparselist = &map->lists[interner_hashmap_hashcode(*map, key)];
	struct interner_hm_pair * pair = interner_sparselist_get(*sparselist, key._ptr);

	if (pair != NULL) {
		unsigned int replaced_value = pair->value;
		pair->value = value;
		return replaced_value;
	}

	interner_sparselist_push(sparselist, key._ptr, value);
	map->total += 1;
	return -1;
}

unsigned int interner_hashmap_remove(struct interner_hashmap * map, const char * key) {
	return INVALID_INTERN_ID;
}
