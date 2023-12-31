#pragma once

#include "common/common.h"

enum Keywords {
    KEYWORD_NOT_FOUND,
    FN,
    CONST,
    LET,
    PACKAGE,
    ELSE,
    IF,
    FOR,
    WHILE,
    DO,
    MATCH,
    STRUCT,
    ENUM,
    TRAIT,
    IMPL,
    RETURN
};

enum Keywords_usage {
    ANY,
    GLOBAL_ONLY,
    FUNCTION_ONLY,
    NONE
};

const static struct Keyword {
    enum Keywords key;
    char flag;
    const char * str;
} conversion [] = {
    {KEYWORD_NOT_FOUND, NONE, "KEYWORD_NOT_FOUND"},
    
    {FN, GLOBAL_ONLY, "fn"},
    {PACKAGE, GLOBAL_ONLY, "package"},
    {STRUCT, GLOBAL_ONLY, "struct"},
    {ENUM, GLOBAL_ONLY, "enum"},
    {TRAIT, GLOBAL_ONLY, "trait"},
    {IMPL, GLOBAL_ONLY, "impl"},

    {CONST, ANY, "const"},
    {LET, ANY, "let"},
    {ELSE, FUNCTION_ONLY, "else"},
    {IF, FUNCTION_ONLY, "if"},
    {FOR, FUNCTION_ONLY, "for"},
    {WHILE, FUNCTION_ONLY, "while"},
    {DO, FUNCTION_ONLY, "do"},
    {MATCH, FUNCTION_ONLY, "match"},
    {RETURN, FUNCTION_ONLY, "return"}
};

struct Keyword str_to_keyword(const char * str);
