#include "checker/context.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"

Context context;

void context_init(ID implicit_cast_trait) {
	context = (Context) {
		.symbol_table = symbol_table_init(),
		.implicit_cast_trait = implicit_cast_trait,
		.implicit_casts = kh_init(map_type_id_to_arena)
	};
}

ID context_get_implicit_cast_trait() {
	return context.implicit_cast_trait;
}

khash_t(map_type_id_to_arena) * context_get_implicit_casts() {
	return &context.implicit_casts;
}

ID context_lookup_declaration(ID name_id) {
	struct symbol_map_entry entry = symbol_map_get_by_name(&context.symbol_table.declarations, name_id);

	if (ID_IS_INVALID(entry.symbol_id)) {
		return INVALID_ID;
	}

	return entry.node_id;
}

ID context_lookup_type(ID name_id) {
	struct symbol_map_entry entry = symbol_map_get_by_name(&context.symbol_table.types, name_id);

	if (ID_IS_INVALID(entry.symbol_id)) {
		return INVALID_ID;
	}

	return entry.node_id;
}

Arena context_lookup_all_declarations(ID name_id) {
	Arena arena = arena_init(sizeof(ID));

	struct symbol_map_entry entry = symbol_map_get_by_name(&context.symbol_table.declarations, name_id);
	if (ID_IS_INVALID(entry.node_id)) {
		return arena;
	}

	ARENA_APPEND(&arena, entry.node_id);

	while (!ID_IS_INVALID(entry.shadowed_symbol_id)) {
		entry = symbol_map_get_by_id(&context.symbol_table.declarations, entry.shadowed_symbol_id);
		ARENA_APPEND(&arena, entry.node_id);
		println("node: {s}", ast_to_string(entry.node_id));
	}

	return arena;
}

void context_add_template_list(Arena arena) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID child_node_id = ARENA_GET(arena, i, ID);
		ASSERT1(ID_IS(child_node_id, ID_AST_SYMBOL)); // should be handled in the parser

		a_symbol symbol = LOOKUP(child_node_id, a_symbol);
		ASSERT1(ID_IS(symbol.name_id, ID_INTERNER)); // should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // should be handled in the parser

		symbol_map_insert(&context.symbol_table.types, symbol.name_id, child_node_id);
	}
}

void context_remove_template_list(Arena arena) {
	for (size_t index_plus_one = arena.size; index_plus_one > 0; --index_plus_one) {
		ID child_node_id = ARENA_GET(arena, index_plus_one - 1, ID);
		ASSERT1(ID_IS(child_node_id, ID_AST_SYMBOL)); // should be handled in the parser

		a_symbol symbol = LOOKUP(child_node_id, a_symbol);
		ASSERT1(ID_IS(symbol.name_id, ID_INTERNER)); // should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // should be handled in the parser

		symbol_map_remove(&context.symbol_table.types, symbol.name_id);
	}
}

void context_add_declaration_list(Arena arena) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID arg_id = ARENA_GET(arena, i, ID);

		switch (arg_id.type) {
			case ID_AST_SYMBOL: {
				a_symbol symbol = LOOKUP(arg_id, a_symbol);
				ASSERT1(!ID_IS_INVALID(symbol.node_id)); // Should be handled in the parser
				ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser
				symbol_map_insert(&context.symbol_table.declarations, symbol.name_id, symbol.node_id);
			} break;
			case ID_AST_FUNCTION: {
				a_function func = LOOKUP(arg_id, a_function);
				symbol_map_insert(&context.symbol_table.declarations, func.name_id, func.info.node_id);
			} break;
			case ID_AST_OP: break;
			default: 
				FATAL("Invalid id type: {s}", id_type_to_string(arg_id.type));
		}
	}
}

void context_remove_declaration_list(Arena arena) {
	for (size_t index_plus_one = arena.size; index_plus_one > 0; --index_plus_one) {
		ID arg_id = ARENA_GET(arena, index_plus_one - 1, ID);

		switch (arg_id.type) {
			case ID_AST_SYMBOL: {
				a_symbol symbol = LOOKUP(arg_id, a_symbol);
				ASSERT1(!ID_IS_INVALID(symbol.node_id));
				ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser

				symbol_map_remove(&context.symbol_table.declarations, symbol.name_id);
			} break;
			case ID_AST_FUNCTION: {
				a_function function = LOOKUP(arg_id, a_function);

				symbol_map_remove(&context.symbol_table.declarations, function.name_id);
			} break;
		}
	}
}

void context_enter_module(a_module module) {
	symbol_table_extend(&context.symbol_table, module.sym_table);
}

void context_exit_module(a_module module) {
	symbol_table_clear(&context.symbol_table);
}

Arena * context_find_implicit_casts(ID type_id) {
	khint_t k = kh_get(map_type_id_to_arena, &context.implicit_casts, type_id);

	if (k == kh_end(&context.implicit_casts)) {
		return NULL;
	}

	return &kh_value(&context.implicit_casts, k);
}

void context_add_implicit_cast(ID type_id, ID implicit_cast) {
	int ret_code;
	khint_t k = kh_put(map_type_id_to_arena, &context.implicit_casts, type_id, &ret_code);

	if (ret_code == KH_PUT_ALREADY_PRESENT) {
		ARENA_APPEND(&kh_value(&context.implicit_casts, k), implicit_cast);
		return;
	}

	ASSERT(ret_code == KH_PUT_SUCCESS, "An error occured while adding implicit cast. Error code {i}", ret_code);

	Arena arena = arena_init(sizeof(ID));
	ARENA_APPEND(&arena, implicit_cast);

	kh_value(&context.implicit_casts, k) = arena;
}

void context_add_implicit_casts(ID type_id, Arena arena) {
	int ret_code;
	khint_t k = kh_put(map_type_id_to_arena, &context.implicit_casts, type_id, &ret_code);

	if (ret_code == KH_PUT_ALREADY_PRESENT) {
		arena_extend(&kh_value(&context.implicit_casts, k), arena);
		return;
	}

	ASSERT(ret_code == KH_PUT_SUCCESS, "An error occured while adding implicit cast. Error code {i}", ret_code);
	kh_value(&context.implicit_casts, k) = arena;
}
