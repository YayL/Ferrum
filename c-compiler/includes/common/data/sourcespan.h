#pragma once

typedef struct source_span {
	const char * start;
	unsigned int length;
} SourceSpan;

#include "common/data/string.h"

#define SOURCE_SPAN_INIT(STR) source_span_init(STR, sizeof(STR) - 1)

SourceSpan source_span_init(const char * start, unsigned int length);
SourceSpan source_span_init_from_string(String string);
char * source_span_to_cstr(SourceSpan span);
