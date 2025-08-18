#include "tables/interner.h"

#include "common/arena.h"
#include "common/hashmap.h"
#include "common/logger.h"
#include "common/string.h"
#include "parser/keywords.h"
#include "parser/operators.h"

#define INTERNER_ID_TO_ARENA_INDEX(ID) ((ID) - 1)
Interner interner;

void interner_init() {
	interner = (Interner) {
		.entries = arena_init(sizeof(struct interner_entry)),
		.map = kh_init(interner_hm),
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
	int ret_code;
	khint_t k = kh_put(interner_hm, &interner.map, string._ptr, &ret_code);

	if (ret_code == KEY_ALREADY_PRESENT) {
		return kh_value(&interner.map, k);
	}

	ASSERT(ret_code == 1, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);

	// +1 so that interner IDs start at 1
	unsigned int ID = interner.entries.size + 1;
	kh_value(&interner.map, k) = ID;
	ARENA_APPEND(&interner.entries, interner_entry_init(ID, string));

	return ID;
}

unsigned int interner_lookup_id(const char * key) {
	khint_t k = kh_get(interner_hm, &interner.map, key);

	if (k == kh_end(&interner.map)) {
		return INVALID_INTERN_ID;
	}

	return kh_value(&interner.map, k);
}

String interner_lookup_str(unsigned int ID) {
	struct interner_entry * entry = arena_get(interner.entries, INTERNER_ID_TO_ARENA_INDEX(ID));
	ASSERT1(entry->ID == ID);
	return entry->str;
}
