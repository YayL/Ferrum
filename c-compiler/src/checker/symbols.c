#include "checker/symbols.h"
#include "codegen/AST.h"
#include "common/logger.h"

char * symbol_expand_path(struct AST * symbol_ast) {
	ASSERT1(symbol_ast->type == AST_SYMBOL);

	String string = string_init_empty();
	a_symbol symbol = symbol_ast->value.symbol;

	for (size_t i = 0; i < symbol.name_ids.size; ++i) {
		ID name_id = ARENA_GET(symbol.name_ids, i, ID);
		ASSERT1(name_id.type == ID_INTERNER);
		string_concat(&string, interner_lookup_str(name_id));
	}

	return string._ptr;
}
