#include "checker/symbol.h"

#include "tables/registry_manager.h"
#include "common/memory/arena.h"

#define SYMBOL_MAP_LOOKUP(SYMBOL, COND, SYMBOL_MAP, NAME_ID) \
	if (COND) { \
		struct symbol_map_entry entry = symbol_map_get_by_name(SYMBOL_MAP, NAME_ID); \
		if (!ID_IS_INVALID(entry.symbol_id)) { \
			return SYMBOL->node_id = entry.node_id; \
		} \
	}

ID qualify_symbol(a_symbol * symbol, enum id_type type_to_find) {
	ASSERT1(symbol->name_ids.size > 0);

	if (!ID_IS_INVALID(symbol->node_id)) {
		return symbol->node_id;
	}

	a_module * module = get_scope(ID_AST_MODULE, symbol->info.scope_id);

	const size_t size = symbol->name_ids.size - 1; // Leave the last name id for after the for loop since all other should just be module traversal
	for (size_t i = 0; i < size; ++i) {
		ID name_id = ARENA_GET(symbol->name_ids, i, ID);

		struct symbol_map_entry entry = symbol_map_get_by_name(&module->sym_table.imports, name_id);
		if (ID_IS_INVALID(entry.symbol_id)) {
			return INVALID_ID;
		}
		ASSERT1(ID_IS(entry.node_id, ID_AST_IMPORT));
		a_import import = LOOKUP(entry.node_id, a_import);
		
		ASSERT1(ID_IS(import.module_id, ID_AST_MODULE));
		module = lookup(import.module_id);
	}

	ID name_id = ARENA_GET(symbol->name_ids, size, ID);

	SYMBOL_MAP_LOOKUP(symbol, type_to_find == ID_INVALID_TYPE || type_to_find == ID_AST_FUNCTION || type_to_find == ID_AST_DECLARATION, &module->sym_table.declarations, name_id);
	SYMBOL_MAP_LOOKUP(symbol, type_to_find == ID_INVALID_TYPE || type_to_find == ID_SYMBOL_TYPE, &module->sym_table.types, name_id);
	SYMBOL_MAP_LOOKUP(symbol, type_to_find == ID_INVALID_TYPE || type_to_find == ID_AST_TRAIT, &module->sym_table.traits, name_id);

	return INVALID_ID;
}
