#pragma once

#include "common/arena.h"
#include "common/string.h"
#include "common/hashmap.h"

#define VALUE_TYPE unsigned int
KHASH_MAP_INIT_STR(interner_hm, VALUE_TYPE)

#define INVALID_INTERN_ID (0)

typedef struct interner {
	struct arena entries;
	khash_t(interner_hm) map;
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
