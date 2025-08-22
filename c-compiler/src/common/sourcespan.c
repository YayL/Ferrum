#include "common/sourcespan.h"
#include <stdlib.h>
#include <string.h>

SourceSpan source_span_init(const char * start, unsigned int length) {
	return (SourceSpan) {
		.start = start,
		.length = length,
	};
}

SourceSpan source_span_init_from_string(String string) {
	return source_span_init(string._ptr, string.length);
}

char * source_span_to_cstr(SourceSpan span) {
	char * cstr = malloc(sizeof(char) * (span.length + 1));

	memcpy(cstr, span.start, sizeof(char) * span.length);
	cstr[span.length] = 0;

	return cstr;
}
