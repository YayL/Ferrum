#pragma once

typedef struct source_span {
	const char * start;
	unsigned int length;
} SourceSpan;

#include "common/string.h"

SourceSpan source_span_init(const char * start, unsigned int length);
SourceSpan source_span_init_from_string(String string);
char * source_span_to_cstr(SourceSpan span);
