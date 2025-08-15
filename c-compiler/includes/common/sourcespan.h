#pragma once

typedef struct source_span {
	const char * start;
	unsigned int length;
} SourceSpan;

SourceSpan source_span_init(const char * start, unsigned int length);
char * source_span_to_cstr(SourceSpan span);
