#include "parser/keywords.h"

#include "tables/interner.h"

struct Keyword get_keyword(unsigned int interner_id) {
    if (interner_id <= ((sizeof(conversion) / sizeof(conversion[0])) - 1)) {
        return conversion[0];
    }

    return conversion[interner_id];
}

unsigned int get_keyword_intern_id(enum Keywords keyword_enum) {
	if (keyword_enum == KEYWORD_NOT_FOUND) {
		return INVALID_INTERN_ID;
	}

	return ((unsigned int) keyword_enum) - 1;
}
