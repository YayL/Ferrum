#pragma once

#include "common/arena.h"
#include "common/string.h"

#define INVALID_INTERN_ID (-1)

struct interner_hm_pair {
	const char * key;
	unsigned int value;
	char is_set;
};

struct interner_sparselist {
	struct interner_hm_pair * buf;
	unsigned int size;
	unsigned int capacity;
    char is_sparse; // if not contiguous memory, meaning there are empty entries
};

struct interner_hashmap {
	struct interner_sparselist * lists;
	size_t bucket_count;
	size_t total;
};

typedef struct interner {
	struct arena entries;
	struct interner_hashmap map;
} Interner;

struct interner_entry {
	String str;
	unsigned int ID;
};

void interner_init();
struct interner_entry interner_entry_init(unsigned int ID, String str);

unsigned int interner_intern(const String string);

unsigned int interner_lookup_id(const char * str);
String interner_lookup_str(unsigned int ID);
