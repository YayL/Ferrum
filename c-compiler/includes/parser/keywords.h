#pragma once

enum Keywords_usage {
    ANY,
    GLOBAL_ONLY,
    FUNCTION_ONLY,
    NONE
};

#define KEYWORDS_LIST(f) \
    f(KEYWORD_NOT_FOUND, NONE, "KEYWORD_NOT_FOUND") \
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

#define KEYWORDS_ENUM_EL(ENUM, ...) ENUM,

enum Keywords {
    KEYWORDS_LIST(KEYWORDS_ENUM_EL)
};

#define CONVERSION_EL(ENUM, USAGE, STR) {ENUM, USAGE, STR},

const static struct Keyword {
    enum Keywords key;
    char flag;
    const char * str;
} conversion [] = {
    KEYWORDS_LIST(CONVERSION_EL)
};

#define GET_KEYWORD_INTERN_ID(ENUM) (ENUM == KEYWORD_NOT_FOUND ? INVALID_INTERN_ID : ((unsigned int) ENUM) - 1)

struct Keyword get_keyword(unsigned int interner_id);
unsigned int get_keyword_intern_id(enum Keywords keyword_enum);
