#include "common/data/string.h"
#include "common/common.h"

void free_string(String string) {
    free((void *) string._ptr);
}

String string_init_empty() {
    return (String) {._ptr = NULL, .length = 0};
}

String string_init_from_source_span(SourceSpan span) {
    return (String) {
        ._ptr = source_span_to_cstr(span),
        .length = span.length,
    };
}

String string_init_with_length(const char * src, size_t length) {
    return string_init_from_source_span(source_span_init(src, length));
}

String string_init(const char * str) {
    return string_init_with_length(str, strlen(str));
}

String string_copy(String src) {
    return string_init_with_length(src._ptr, src.length);
}

void string_append(String * dest, const char * to_append) {
    size_t append_size = strlen(to_append);
    size_t new_size = dest->length + append_size;

    if (dest->_ptr == NULL) {
        dest->_ptr = malloc(sizeof(char) * (new_size + 1));
    } else {
        dest->_ptr = realloc(dest->_ptr, sizeof(char) * (new_size + 1));
    }
    memcpy(dest->_ptr + dest->length, to_append, append_size);
    dest->_ptr[new_size] = 0;

    dest->length = new_size;
}

void string_concat(String * dest, String to_append) {
    string_concat_span(dest, source_span_init(to_append._ptr, to_append.length));
}

void string_concat_span(String * dest, SourceSpan span) {
    size_t new_size = dest->length + span.length;

    if (dest->_ptr == NULL) {
        dest->_ptr = malloc(sizeof(char) * new_size);
    } else {
        dest->_ptr = realloc(dest->_ptr, sizeof(char) * new_size);
    }
    memcpy(dest->_ptr + dest->length, span.start, span.length);
    dest->_ptr[new_size] = 0;

    dest->length = new_size;
}

void string_cut(String * dest, int count) {
    if (dest->_ptr == NULL || count > dest->length) {
        FATAL("string_cut is cutting away more than the length of the string");
    }
    
    dest->length -= count;
    dest->_ptr = realloc(dest->_ptr, (dest->length + 1) * sizeof(char));
    dest->_ptr[dest->length] = 0;
}
