#include "tables/interner.h"

#include "common/arena.h"
#include "common/logger.h"
#include "common/string.h"
#include "parser/keywords.h"
#include "parser/operators.h"
#include <stdlib.h>
#include <string.h>

struct interner_hashmap interner_hashmap_init(size_t pow_capacity);
unsigned int interner_hashmap_set(struct interner_hashmap map, const char * key, unsigned int value);
unsigned int interner_hashmap_get(struct interner_hashmap map, const char * key);
unsigned int interner_hashmap_get_or_set(struct interner_hashmap map, const char * key, unsigned int value);

Interner interner;

/* ================================ */
/* ====== INTERNER FUNCTIONS ====== */
/* ================================ */

#define INTERNER_ID_TO_ARENA_INDEX(ID) ((ID) - 1)

void interner_init() {
	interner = (Interner) {
		.entries = arena_init(sizeof(struct interner_entry)),
		.map = interner_hashmap_init(5),
	};

	keywords_intern();
	operators_intern();
}

struct interner_entry interner_entry_init(unsigned int ID, String str) {
	return (struct interner_entry) {
		.ID = ID,
		.str = str,
	};
}

unsigned int interner_intern(String string) {
	// +1 so that interner IDs start at 1
	unsigned int ID = interner_hashmap_get_or_set(interner.map, string._ptr, interner.entries.size + 1);

	// If a new ID has been produced. As long as we do not remove any interned strings the highest
	// recorded ID should always be the same interned string count
	if (interner.entries.size < ID) {
		ARENA_APPEND(&interner.entries, interner_entry_init(ID, string));
	}

	return ID;
}

unsigned int interner_lookup_id(const char * str) {
	return interner_hashmap_get(interner.map, str);
}

String interner_lookup_str(unsigned int ID) {
	struct interner_entry * entry = arena_get(interner.entries, INTERNER_ID_TO_ARENA_INDEX(ID));
	ASSERT1(entry->ID == ID);
	return entry->str;
}

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
		sparselist->capacity = 4;
		sparselist->buf = calloc(sparselist->capacity, sizeof(struct interner_hm_pair));
	} else if (sparselist->capacity <= sparselist->size) {
		sparselist->capacity *= 2;
		ASSERT1(sparselist->size < sparselist->capacity);
		sparselist->buf = realloc(sparselist->buf, sparselist->capacity * sizeof(struct interner_hm_pair));
		sparselist->is_sparse = 0;
		memset(&sparselist->buf[sparselist->size], 0, (sparselist->capacity - sparselist->size) * sizeof(struct interner_hm_pair));
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
unsigned int interner_hashmap_hashcode(const char * key) {
	unsigned int hash = 2166136261;
	while (*key) {
		hash ^= (unsigned char) (*key++);
		hash *= 16777619;
	}
	return hash;
}

unsigned int interner_hashmap_key_to_bucket_index(struct interner_hashmap map, const char * key) {
	return interner_hashmap_hashcode(key) & (map.bucket_count - 1);
}

unsigned int interner_hashmap_get_or_set(struct interner_hashmap map, const char * key, unsigned int value) {
	unsigned int bucket = interner_hashmap_key_to_bucket_index(map, key);
	struct interner_sparselist * sparselist = &map.lists[bucket];
	struct interner_hm_pair * pair = interner_sparselist_get(*sparselist, key);

	if (pair != NULL) {
		return pair->value;
	}

	interner_sparselist_push(sparselist, key, value);
	return value;
}

unsigned int interner_hashmap_get(struct interner_hashmap map, const char * key) {
	struct interner_hm_pair * pair = interner_sparselist_get(map.lists[interner_hashmap_key_to_bucket_index(map, key)], key);
	if (pair == NULL) {
		return INVALID_INTERN_ID;
	}

	return pair->value;
}

char interner_hashmap_has(struct interner_hashmap map, const char * key) {
	return interner_sparselist_get(map.lists[interner_hashmap_key_to_bucket_index(map, key)], key) != NULL;
}

unsigned int interner_hashmap_set(struct interner_hashmap map, const char * key, unsigned int value) {
	struct interner_sparselist * sparselist = &map.lists[interner_hashmap_key_to_bucket_index(map, key)];
	struct interner_hm_pair * pair = interner_sparselist_get(*sparselist, key);

	if (pair != NULL) {
		unsigned int replaced_value = pair->value;
		pair->value = value;
		return replaced_value;
	}

	interner_sparselist_push(sparselist, key, value);

	return INVALID_INTERN_ID;
}

unsigned int interner_hashmap_remove(struct interner_hashmap * map, const char * key) {
	return INVALID_INTERN_ID;
}
