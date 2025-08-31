#include "tables/interner.h"

#include "common/hashmap.h"
#include "parser/keywords.h"
#include "parser/operators.h"
#include "tables/registry_manager.h"

#define INTERNER_ID_TO_ARENA_INDEX(ID) ((ID.id) - 1)
Interner interner;

void interner_init() {
	interner = (Interner) {
		.map = kh_init(map_string_to_id),
	};

	keywords_intern();
	operators_intern();
}

ID interner_intern(String string) {
	int ret_code;
	khint_t k = kh_put(map_string_to_id, &interner.map, string._ptr, &ret_code);

	if (ret_code == KEY_ALREADY_PRESENT) {
		return kh_value(&interner.map, k);
	}

	ASSERT(ret_code == 1, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);

	struct interner_entry * entry = interner_allocate();
	entry->str = string;
	kh_value(&interner.map, k) = entry->id;

	return entry->id;
}

ID interner_lookup_id(const char * key) {
	khint_t k = kh_get(map_string_to_id, &interner.map, key);

	if (k == kh_end(&interner.map)) {
		return INVALID_ID;
	}

	return kh_value(&interner.map, k);
}

String interner_lookup_str(ID interner_id) {
	return interner_lookup(interner_id)->str;
}
