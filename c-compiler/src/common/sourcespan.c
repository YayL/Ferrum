#include "common/sourcespan.h"
#include <stdlib.h>
#include <string.h>

SourceSpan source_span_init(const char * start, unsigned int length) {
	return (SourceSpan) {
		.start = start,
		.length = length,
	};
}

char * source_span_to_cstr(SourceSpan span) {
	char * cstr = malloc(sizeof(char) * (span.length + 1));

	memcpy(cstr, span.start, sizeof(char) * span.length);
	cstr[span.length] = 0;

	return cstr;
}
