#include "common/string.h"

void free_string(String ** string) {
    free((*string)->_ptr);
    free(*string);
    *string = NULL;
}

String * init_string_with_length(const char * str, size_t size) {
    String * string = malloc(sizeof(String));
    
    string->size = size;
    string->_ptr = malloc(sizeof(char) * (size + 1));
    memcpy(string->_ptr, str, size);
    string->_ptr[size] = 0;

    return string;
}

String * init_string(const char * str) {
    return init_string_with_length(str, strlen(str));
}

String * string_copy(String * src) {
    return init_string_with_length(src->_ptr, src->size);
}

void string_append(String * dest, const char * to_append) {
    size_t append_size = strlen(to_append);
    size_t new_size = dest->size + append_size;

    dest->_ptr = realloc(dest->_ptr, sizeof(char) * (new_size + 1));
    memcpy(dest->_ptr + dest->size, to_append, append_size);
    dest->_ptr[new_size] = 0;

    dest->size = new_size;
}

void string_concat(String * dest, String * to_append) {
    size_t append_size = to_append->size - 1; // this might be have some sort of offset error so use with caution
    size_t new_size = dest->size + append_size;

    dest->_ptr = realloc(dest->_ptr, sizeof(char) * new_size);
    memcpy(dest->_ptr + dest->size, to_append->_ptr, append_size);
    dest->_ptr[new_size] = 0;

    dest->size = new_size;
}

void string_cut(String * dest, int count) {
    if (count > dest->size) {
        FATAL("string_cut is cutting away more than the length of the string");
    }

    dest->size -= count;
    dest->_ptr = realloc(dest->_ptr, (dest->size + 1) * sizeof(char));
    dest->_ptr[dest->size] = 0;
}
