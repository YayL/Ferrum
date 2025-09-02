#include "tables/interner.h"

#include "common/hashmap.h"
#include "parser/keywords.h"
#include "parser/operators.h"
#include "tables/registry_manager.h"

#define INTERNER_ID_TO_ARENA_INDEX(ID) ((ID.id) - 1)
Interner interner;

void interner_init() {
	interner = (Interner) {
		.map = kh_init(map_bstring_to_id),
	};

	keywords_intern();
	operators_intern();
}

ID interner_intern(SourceSpan span) {
	BString temp_bstring = { ._ptr = span.start, .length = span.length };
	khint_t k = kh_get(map_bstring_to_id, &interner.map, temp_bstring); 

	if (k != kh_end(&interner.map)) {
		return kh_value(&interner.map, k);
	}

	BString bstring = buffer_string_init_from_source_span(span);
	int ret_code;
	k = kh_put(map_bstring_to_id, &interner.map, bstring, &ret_code);

	ASSERT(ret_code != KEY_ALREADY_PRESENT, "Key already exists but was not retrieved when using get???");
	ASSERT(ret_code == 1, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);

	struct interner_entry * entry = interner_allocate();
	entry->str = bstring;
	kh_value(&interner.map, k) = entry->id;

	return entry->id;
}

ID interner_lookup_id(const SourceSpan key) {
	BString temp_bstring = { ._ptr = key.start, .length = key.length };
	khint_t k = kh_get(map_bstring_to_id, &interner.map, temp_bstring);

	if (k == kh_end(&interner.map)) {
		return INVALID_ID;
	}

	return kh_value(&interner.map, k);
}

BString interner_lookup_str(ID interner_id) {
	return interner_lookup(interner_id)->str;
}
