#include "checker/symbol.h"

#include "tables/registry_manager.h"
#include "common/memory/arena.h"

#define SYMBOL_MAP_LOOKUP(COND, SYMBOL_MAP, NAME_ID) \
	if (COND) { \
		struct symbol_map_entry entry = symbol_map_get_by_name(SYMBOL_MAP, NAME_ID); \
		if (!ID_IS_INVALID(entry.symbol_id)) { \
			return entry.node_id; \
		} \
	}

ID module_lookup_id(a_module * module, ID name_id, enum id_type type_to_find) {
	SYMBOL_MAP_LOOKUP(type_to_find == ID_INVALID_TYPE || type_to_find == ID_AST_FUNCTION || type_to_find == ID_AST_DECLARATION, &module->sym_table.declarations, name_id);
	SYMBOL_MAP_LOOKUP(type_to_find == ID_INVALID_TYPE || type_to_find == ID_SYMBOL_TYPE, &module->sym_table.types, name_id);
	SYMBOL_MAP_LOOKUP(type_to_find == ID_INVALID_TYPE || type_to_find == ID_SYMBOL_TYPE || type_to_find == ID_AST_TRAIT, &module->sym_table.traits, name_id);

	return INVALID_ID;
}

ID qualify_symbol(a_symbol * symbol, enum id_type type_to_find) {
	ASSERT1(symbol->name_ids.size > 0);

	if (!ID_IS_INVALID(symbol->node_id)) {
		return symbol->node_id;
	}

	a_module * module = get_scope(ID_AST_MODULE, symbol->info.scope_id);

	size_t name_ids_stepped = symbol->name_ids.size;
	for (size_t i = 0; i < symbol->name_ids.size; ++i) {
		ID name_id = ARENA_GET(symbol->name_ids, i, ID);

		struct symbol_map_entry entry = symbol_map_get_by_name(&module->sym_table.imports, name_id);
		if (ID_IS_INVALID(entry.symbol_id)) {
			name_ids_stepped = i;
			break;
		}

		ASSERT1(ID_IS(entry.node_id, ID_AST_IMPORT));
		a_import import = LOOKUP(entry.node_id, a_import);
		
		ASSERT1(ID_IS(import.module_id, ID_AST_MODULE));
		module = lookup(import.module_id);
	}

	ID name_id = ARENA_GET(symbol->name_ids, name_ids_stepped, ID);
	ID next_id = module_lookup_id(module, name_id, type_to_find);

	// Last name id
	if (name_ids_stepped + 1 == symbol->name_ids.size) {
		return symbol->node_id = next_id;
	} else if (name_ids_stepped + 2 < symbol->name_ids.size) {
		return INVALID_ID;
	}

	next_id = module_lookup_id(module, name_id, type_to_find != ID_SYMBOL_TYPE ? ID_SYMBOL_TYPE : ID_AST_DECLARATION);

	if (ID_IS_INVALID(next_id)) {
		return INVALID_ID;
	}

	for (size_t i = name_ids_stepped + 1; i < symbol->name_ids.size; ++i) {
		ID next_name_id = ARENA_GET(symbol->name_ids, i, ID);

		switch (next_id.type) {
			case ID_AST_STRUCT: {
				a_structure _struct = LOOKUP(next_id, a_structure);

				for (size_t j = 0; j < _struct.members.size; ++j) {
					ID struct_child_id = ARENA_GET(_struct.members, j, ID);

					// print_ast_tree(struct_child_id);

					switch (struct_child_id.type) {
						case ID_AST_FUNCTION: {
							a_function func = LOOKUP(struct_child_id, a_function);

							if (id_is_equal(func.name_id, next_name_id)) {
								return symbol->node_id = func.info.node_id;
							}
						} break;
						case ID_AST_SYMBOL: {
							a_symbol sym = LOOKUP(struct_child_id, a_symbol);

							if (id_is_equal(sym.name_id, next_name_id)) {
								return symbol->node_id = sym.info.node_id;
							}
						} break;
						default:
							FATAL("Unimplemented type: {s}", id_type_to_string(struct_child_id.type));
					}
				}
			} break;
			default:
				FATAL("Unimplemented type: {s}", id_type_to_string(next_id.type));
		}
	}

	return INVALID_ID;
}

#define DECLARATION_CHECK_IF_TYPE(DECLARATION, NAME_ID, TYPE) \
	TYPE type = LOOKUP(DECLARATION, TYPE); \
	if (id_is_equal(type.name_id, NAME_ID)) { return DECLARATION; }

ID qualify_declaration(Arena declarations, ID declaration_name_id) {
	for (size_t i = 0; i < declarations.size; ++i) {
		ID declaration = ARENA_GET(declarations, i, ID);
		switch (declaration.type) {
			case ID_AST_FUNCTION: {
				DECLARATION_CHECK_IF_TYPE(declaration, declaration_name_id, a_function);
			} break;
			case ID_AST_SYMBOL: {
				DECLARATION_CHECK_IF_TYPE(declaration, declaration_name_id, a_symbol);
			} break;
			case ID_AST_DECLARATION: {
				DECLARATION_CHECK_IF_TYPE(declaration, declaration_name_id, a_declaration);
			} break;
			default:
				FATAL("Not implemented: {s}", id_type_to_string(declaration.type));
		}
	}

	return INVALID_ID;
}
