#pragma once

#include "common/arena.h"
#include "common/string.h"
#include "common/ID.h"

typedef struct interner {
	Arena entries;
	khash_t(map_string_to_id) map;
} Interner;

struct interner_entry {
	String str;
	ID id;
};

void interner_init();
struct interner_entry interner_entry_init(ID id, String str);

ID interner_intern(const String string);

ID interner_lookup_id(const char * str);
String interner_lookup_str(ID id);
