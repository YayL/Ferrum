#pragma once

#include "tables/interner.h"
enum Keywords_usage {
    ANY,
    GLOBAL_ONLY,
    FUNCTION_ONLY,
    NONE
};

#define KEYWORDS_LIST(f) \
    f(KEYWORD_BOOL, NONE, "bool") \
    \
    f(KEYWORD_CONST, ANY, "const") \
    f(KEYWORD_LET, ANY, "let") \
    \
    f(KEYWORD_FN, GLOBAL_ONLY, "fn") \
    f(KEYWORD_PACKAGE, GLOBAL_ONLY, "package") \
    f(KEYWORD_STRUCT, GLOBAL_ONLY, "struct") \
    f(KEYWORD_ENUM, GLOBAL_ONLY, "enum") \
    f(KEYWORD_TRAIT, GLOBAL_ONLY, "trait") \
    f(KEYWORD_IMPL, GLOBAL_ONLY, "impl") \
    f(KEYWORD_INLINE, GLOBAL_ONLY, "inline") \
    \
    f(KEYWORD_ELSE, FUNCTION_ONLY, "else") \
    f(KEYWORD_IF, FUNCTION_ONLY, "if") \
    f(KEYWORD_FOR, FUNCTION_ONLY, "for") \
    f(KEYWORD_WHILE, FUNCTION_ONLY, "while") \
    f(KEYWORD_DO, FUNCTION_ONLY, "do") \
    f(KEYWORD_MATCH, FUNCTION_ONLY, "match") \
    f(KEYWORD_RETURN, FUNCTION_ONLY, "return") \
    f(KEYWORD_TYPENAME, FUNCTION_ONLY, "typename")

#define KEYWORDS_LIST_FULL(f) \
    f(KEYWORD_NOT_FOUND, NONE, "KEYWORD_NOT_FOUND") \
    KEYWORDS_LIST(f)

#define KEYWORDS_ENUM_EL(ENUM, ...) ENUM,

enum Keywords {
    KEYWORDS_LIST_FULL(KEYWORDS_ENUM_EL)
};

#define CONVERSION_EL(ENUM, USAGE, STR) {INVALID_INTERN_ID, ENUM, USAGE},

struct Keyword {
    unsigned int intern_id;
    const enum Keywords key;
    const char flag;
};

void keywords_intern();

struct Keyword keyword_get(enum Keywords keyword_enum);
struct Keyword keyword_get_by_intern_id(unsigned int ID);

unsigned int keyword_get_intern_id(enum Keywords keyword_enum);
const char * keyword_get_str(enum Keywords keyword_enum);
