#include "parser/keywords.h"

#include "common/string.h"
#include "tables/interner.h"

#define CONVERSION_EL(ENUM, USAGE, STR) {INVALID_INTERN_ID, ENUM, USAGE},
struct Keyword keywords[] = {
    KEYWORDS_LIST_FULL(CONVERSION_EL)
};
#define KEYWORDS_LIST_COUNT (sizeof(keywords) / sizeof(keywords[0]))

#define GET_KEYWORD(KEY) keywords[KEY]

#define KEYWORD_GET_STR(ENUM_KEY, USAGE, STR) case ENUM_KEY: return STR;
const char * keyword_get_str(enum Keywords keyword_enum) {
    switch (keyword_enum) {
        KEYWORDS_LIST_FULL(KEYWORD_GET_STR)
    }
}

struct Keyword keyword_get(enum Keywords keyword_enum) {
    if (KEYWORDS_LIST_COUNT < keyword_enum) {
        return GET_KEYWORD(KEYWORD_NOT_FOUND);
    }

    struct Keyword keyword = GET_KEYWORD(keyword_enum);
    ASSERT1(keyword.key == keyword_enum);
    return keyword;
}

char keyword_interner_id_is_inbounds(unsigned int ID) {
    return GET_KEYWORD(KEYWORD_NOT_FOUND + 1).intern_id <= ID && ID <= GET_KEYWORD(KEYWORDS_LIST_COUNT - 1).intern_id;
}

struct Keyword keyword_get_by_intern_id(unsigned int ID) {
    struct Keyword keyword = keyword_get(ID - keyword_get(KEYWORD_NOT_FOUND + 1).intern_id + 1);
    ASSERT1(keyword.key == KEYWORD_NOT_FOUND || keyword.intern_id == ID);
    return keyword;
}

unsigned int keyword_get_intern_id(enum Keywords keyword_enum) {
	return keyword_get(keyword_enum).intern_id;
}

#define INTERN_KEYWORD(ENUM_KEY, USAGE, STR) \
    GET_KEYWORD(ENUM_KEY).intern_id = interner_intern(STRING_FROM_LITERAL(STR));
#define PRINT_INTERNED_KEYWORD(ENUM_KEY, USAGE, STR) \
    println("{u}: '{s}'", GET_KEYWORD(ENUM_KEY).intern_id, STR);

void keywords_intern() {
    KEYWORDS_LIST(INTERN_KEYWORD)
}

void keywords_print() {
    KEYWORDS_LIST(PRINT_INTERNED_KEYWORD);
}
