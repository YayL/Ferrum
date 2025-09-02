#include "checker/symbols.h"

#include "tables/interner.h"

char * symbol_expand_path(a_symbol symbol) {
	String string = string_init_empty();

	for (size_t i = 0; i < symbol.name_ids.size; ++i) {
		ID name_id = ARENA_GET(symbol.name_ids, i, ID);
		ASSERT1(name_id.type == ID_INTERNER);
		string_concat_span(&string, buffer_string_to_source_span(interner_lookup_str(name_id)));
	}

	return string._ptr;
}
