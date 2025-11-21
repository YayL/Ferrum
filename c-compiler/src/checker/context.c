#include "checker/context.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"

Context context;

void context_init(ID implicit_cast_trait) {
	context = (Context) {
		.symbol_table = symbol_table_init(),
		.implicit_cast_trait = implicit_cast_trait 
	};
}

ID context_get_implicit_cast_trait() {
	return context.implicit_cast_trait;
}

ID context_lookup_declaration(ID name_id) {
	struct symbol_map_entry entry = symbol_map_get_by_name(&context.symbol_table.declarations, name_id);

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
		ASSERT1(ID_IS(arg_id, ID_AST_SYMBOL)); // Should be handled in the parser

		a_symbol symbol = LOOKUP(arg_id, a_symbol);
		ASSERT1(!ID_IS_INVALID(symbol.node_id)); // Should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser

		// println("Trying to add symbol: {s}", interner_lookup_str(symbol.name_id)._ptr);
		symbol_map_insert(&context.symbol_table.declarations, symbol.name_id, symbol.node_id);
	}
}

void context_remove_declaration_list(Arena arena) {
	for (size_t index_plus_one = arena.size; index_plus_one > 0; --index_plus_one) {
		ID arg_id = ARENA_GET(arena, index_plus_one - 1, ID);
		ASSERT1(ID_IS(arg_id, ID_AST_SYMBOL)); // Should be handled in the parser

		a_symbol symbol = LOOKUP(arg_id, a_symbol);
		ASSERT1(!ID_IS_INVALID(symbol.node_id)); // Should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser

		// println("Trying to remove symbol: {s}", interner_lookup_str(symbol.name_id)._ptr);
		symbol_map_remove(&context.symbol_table.declarations, symbol.name_id);
	}
}

void context_enter_module(a_module module) {
	symbol_table_extend(&context.symbol_table, module.sym_table);
}

void context_exit_module(a_module module) {
	symbol_table_clear(&context.symbol_table);
}

void context_enter_function(a_function function) {
	// println("Entering: '{s}'", interner_lookup_str(function.name_id)._ptr);
	context_add_template_list(function.templates);

	ASSERT1(ID_IS(function.arguments_id, ID_AST_EXPR));
	a_expression arguments = LOOKUP(function.arguments_id, a_expression);
	context_add_declaration_list(arguments.children);
}

void context_exit_function(a_function function) {
	// println("Leaving: '{s}'", interner_lookup_str(function.name_id)._ptr);
	ASSERT1(ID_IS(function.arguments_id, ID_AST_EXPR));
	a_expression arguments = LOOKUP(function.arguments_id, a_expression);
	context_remove_declaration_list(arguments.children);
	context_remove_template_list(function.templates);
}

void context_enter_scope(a_scope scope) {
	// println("Scope declarations: {u}", scope.declarations.size);
	context_add_declaration_list(scope.declarations);
}

void context_exit_scope(a_scope scope) {
	context_remove_declaration_list(scope.declarations);
}
