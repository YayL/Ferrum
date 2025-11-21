#pragma once

#include "common/ID.h"

enum Keywords_usage {
    ANY,
    GLOBAL_ONLY,
    FUNCTION_ONLY,
    NONE
};

#define KEYWORDS_LIST(f) \
    f(KEYWORD_BOOL, NONE, "bool") \
    f(KEYWORD_PLACE, NONE, "Place") \
    f(KEYWORD_MUTPLACE, NONE, "MutPlace") \
    f(KEYWORD_SELF, NONE, "Self") \
    f(KEYWORD_STATIC, NONE, "static") \
    f(KEYWORD_VOID, NONE, "void") \
    \
    f(KEYWORD_LET, ANY, "let") \
    f(KEYWORD_MUT, ANY, "mut") \
    \
    f(KEYWORD_FN, GLOBAL_ONLY, "fn") \
    f(KEYWORD_IMPORT, GLOBAL_ONLY, "import") \
    f(KEYWORD_STRUCT, GLOBAL_ONLY, "struct") \
    f(KEYWORD_ENUM, GLOBAL_ONLY, "enum") \
    f(KEYWORD_TRAIT, GLOBAL_ONLY, "trait") \
    f(KEYWORD_IMPL, GLOBAL_ONLY, "impl") \
    f(KEYWORD_INLINE, GLOBAL_ONLY, "inline") \
    f(KEYWORD_AS, GLOBAL_ONLY, "as") \
    f(KEYWORD_GROUP, GLOBAL_ONLY, "group") \
    f(KEYWORD_WHERE, GLOBAL_ONLY, "where") \
    \
    f(KEYWORD_ELSE, FUNCTION_ONLY, "else") \
    f(KEYWORD_IF, FUNCTION_ONLY, "if") \
    f(KEYWORD_FOR, FUNCTION_ONLY, "for") \
    f(KEYWORD_WHILE, FUNCTION_ONLY, "while") \
    f(KEYWORD_DO, FUNCTION_ONLY, "do") \
    f(KEYWORD_MATCH, FUNCTION_ONLY, "match") \
    f(KEYWORD_RETURN, FUNCTION_ONLY, "return")

#define KEYWORDS_LIST_FULL(f) \
    f(KEYWORD_NOT_FOUND, NONE, "KEYWORD_NOT_FOUND") \
    KEYWORDS_LIST(f)

#define KEYWORDS_ENUM_EL(ENUM, ...) ENUM,

enum Keywords {
    KEYWORDS_LIST_FULL(KEYWORDS_ENUM_EL)
};

struct Keyword {
    ID intern_id;
    const enum Keywords key;
    const char flag;
};

void keywords_intern();

struct Keyword keyword_get(enum Keywords keyword_enum);
struct Keyword keyword_get_by_intern_id(ID id);

char keyword_interner_id_is_inbounds(ID id);
ID keyword_get_intern_id(enum Keywords keyword_enum);
const char * keyword_get_str(enum Keywords keyword_enum);
