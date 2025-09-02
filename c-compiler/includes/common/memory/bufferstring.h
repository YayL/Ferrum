#pragma once

#include "common/common.h"

typedef struct {
	char * _ptr;
	size_t length;
} BString;

#include "common/data/sourcespan.h"
#include "common/memory/buffer.h"

#define BUFFER_BUFFER_STRING_FROM_LITERAL(STR) buffer_string_init_with_length(STR, (sizeof(STR) / sizeof(char)) - 1)

uint32_t get_buffer_string_inits();

static BString buffer_string_init_from_source_span(SourceSpan span) {
	char * str = buffer_blocks_allocate(span.length + 1);

	memcpy(str, span.start, span.length);
	str[span.length] = 0;

	return (BString) { ._ptr = str, .length = span.length };
}

static inline BString buffer_string_init_with_length(const char * src, size_t length) {
	return buffer_string_init_from_source_span(source_span_init(src, length));
}

static inline BString buffer_string_init_from_string(String string) {
	return buffer_string_init_from_source_span(source_span_init(string._ptr, string.length));
}

static inline SourceSpan buffer_string_to_source_span(BString bstring) {
	return (SourceSpan) { .start = bstring._ptr, .length = bstring.length };
}

#include "common/data/hashmap.h"

static kh_inline khint_t _bstring_hash(BString string) {
	uint32_t h = 2166136261u;         // FNV offset basis
	for (size_t i = 0; i < string.length; i++) {
		h ^= (uint8_t) string._ptr[i];
		h *= 16777619u;                 // FNV prime
	}
	return h;
}

static kh_inline char _bstring_is_equal(BString string1, BString string2) {
	return string1.length == string2.length && string1._ptr[0] == string2._ptr[0] && !strncmp(&string1._ptr[1], &string2._ptr[1], string1.length - 1);
}

#include "common/ID.h"
KHASH_INIT(map_bstring_to_id, BString, ID, KHASH_IS_MAP, _bstring_hash, _bstring_is_equal);
