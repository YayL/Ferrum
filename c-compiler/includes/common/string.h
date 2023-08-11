#pragma once

#include "common/common.h"

typedef struct string {
    char * _ptr;
    size_t size;
} String;

void free_string(String **);

String * init_string_with_length(const char * str, size_t size);
String * init_string(const char * src);
String * string_copy(String * src);

void string_append(String * dest, const char * to_append);
void string_concat(String * dest, String * to_append);
void string_cut(String * dest, int count);
