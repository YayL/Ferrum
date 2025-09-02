#pragma once

#include "common/ID.h"
#include "common/memory/bufferstring.h"

typedef struct interner {
	khash_t(map_bstring_to_id) map;
} Interner;

typedef struct interner_entry {
	BString str;
	ID id;
} interner_entry;

void interner_init();
struct interner_entry interner_entry_init(ID id, String str);

ID interner_intern(const SourceSpan span);

ID interner_lookup_id(const SourceSpan span);
BString interner_lookup_str(ID id);
