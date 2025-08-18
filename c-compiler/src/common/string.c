#include "common/string.h"
#include "common/sourcespan.h"
#include <stdlib.h>

void free_string(String string) {
    free((void *) string._ptr);
}

String init_string_from_source_span(SourceSpan span) {
    return (String) {
        ._ptr = source_span_to_cstr(span),
        .length = span.length,
    };
}

String init_string_with_length(const char * src, size_t length) {
    return init_string_from_source_span(source_span_init(src, length));
}

String init_string(const char * str) {
    return init_string_with_length(str, strlen(str));
}

String string_copy(String src) {
    return init_string_with_length(src._ptr, src.length);
}

void string_append(String * dest, const char * to_append) {
    size_t append_size = strlen(to_append);
    size_t new_size = dest->length + append_size;

    dest->_ptr = realloc(dest->_ptr, sizeof(char) * (new_size + 1));
    memcpy(dest->_ptr + dest->length, to_append, append_size);
    dest->_ptr[new_size] = 0;

    dest->length = new_size;
}

void string_concat(String * dest, String * to_append) {
    size_t append_size = to_append->length - 1; // this might be have some sort of offset error so use with caution
    size_t new_size = dest->length + append_size;

    dest->_ptr = realloc(dest->_ptr, sizeof(char) * new_size);
    memcpy(dest->_ptr + dest->length, to_append->_ptr, append_size);
    dest->_ptr[new_size] = 0;

    dest->length = new_size;
}

void string_cut(String * dest, int count) {
    if (count > dest->length) {
        FATAL("string_cut is cutting away more than the length of the string");
    }

    dest->length -= count;
    dest->_ptr = realloc(dest->_ptr, (dest->length + 1) * sizeof(char));
    dest->_ptr[dest->length] = 0;
}
