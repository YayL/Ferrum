#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct string {
    char * _ptr;
    size_t length;
} String;

#include "common/sourcespan.h"

#define STRING_FROM_LITERAL(STR) string_init_with_length(STR, (sizeof(STR) / sizeof(char)) - 1)

void free_string(String);

String string_init_empty();
String string_init_from_source_span(SourceSpan span);
String string_init_with_length(const char * src, size_t length);
String string_init(const char * src);

String string_copy(String src);

void string_append(String * dest, const char * to_append);
void string_concat(String * dest, String to_append);
void string_concat_span(String * dest, SourceSpan span);
void string_cut(String * dest, int count);
