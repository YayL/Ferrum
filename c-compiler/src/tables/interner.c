#include "tables/interner.h"

#include "common/arena.h"
#include "common/hashmap.h"
#include "common/logger.h"
#include "common/string.h"
#include "parser/keywords.h"
#include "parser/operators.h"

#define INTERNER_ID_TO_ARENA_INDEX(ID) ((ID.id) - 1)
Interner interner;

void interner_init() {
	interner = (Interner) {
		.entries = arena_init(sizeof(struct interner_entry)),
		.map = kh_init(map_string_to_id),
	};

	keywords_intern();
	operators_intern();
}

struct interner_entry interner_entry_init(ID id, String str) {
	return (struct interner_entry) {
		.id = id,
		.str = str,
	};
}

ID interner_intern(String string) {
	int ret_code;
	khint_t k = kh_put(map_string_to_id, &interner.map, string._ptr, &ret_code);

	if (ret_code == KEY_ALREADY_PRESENT) {
		return kh_value(&interner.map, k);
	}

	ASSERT(ret_code == 1, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);

	// +1 so that interner IDs start at 1
	ID id = id_init(interner.entries.size + 1, ID_INTERNER);
	kh_value(&interner.map, k) = id;
	ARENA_APPEND(&interner.entries, interner_entry_init(id, string));

	return id;
}

ID interner_lookup_id(const char * key) {
	khint_t k = kh_get(map_string_to_id, &interner.map, key);

	if (k == kh_end(&interner.map)) {
		return INVALID_ID;
	}

	return kh_value(&interner.map, k);
}

String interner_lookup_str(ID id) {
	struct interner_entry * entry = arena_get_ref(interner.entries, INTERNER_ID_TO_ARENA_INDEX(id));
	ASSERT1(id_is_equal(entry->id, id));
	return entry->str;
}
