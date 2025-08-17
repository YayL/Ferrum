#pragma once

#include "common/common.h"

typedef struct string {
    char * _ptr;
    size_t length;
} String;

#define STRING_FROM_LITERAL(STR) init_string_with_length(STR, (sizeof(STR) / sizeof(char)) - 1)

void free_string(String);

String init_string_with_length(const char * src, size_t length);
String init_string(const char * src);

String string_copy(String src);

void string_append(String * dest, const char * to_append);
void string_concat(String * dest, String * to_append);
void string_cut(String * dest, int count);
