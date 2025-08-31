#include "checker/context.h"

#include "checker/symbols.h"
#include "parser/AST.h"
#include "tables/registry_manager.h"

Context context;

void context_init() {
	context = (Context) {
		.declarations = symbol_table_init(),
		.traits = symbol_table_init(),
		.types = symbol_table_init(),
		.imports = symbol_table_init(),
	};
}

// These should perhaps not be a thing. Modules should maybe just have their own symbol tables instead, it would help with a lot of things tbh
void context_enter_module(a_module module) {
	for (size_t i = 0; i < module.members.size; ++i) {
		ID child_node_id = ARENA_GET(module.members, i, ID);
		unsigned int name_id;
		struct symbol_table * sym_table;

		switch (child_node_id.type) {
			case ID_AST_DECLARATION: {
				a_declaration declaration = LOOKUP(child_node_id, a_declaration);
				symbol_table_insert(&context.declarations, declaration.name_id, child_node_id); break;
			}
			case ID_AST_FUNCTION: {
				a_function function = LOOKUP(child_node_id, a_function);
				symbol_table_insert(&context.declarations, function.name_id, child_node_id); break;
			}
			case ID_AST_STRUCT: {
				a_structure structure = LOOKUP(child_node_id, a_structure);
				symbol_table_insert(&context.types, structure.name_id, child_node_id); break;
			}
			case ID_AST_ENUM: {
				a_enumeration enumeration = LOOKUP(child_node_id, a_enumeration);
				symbol_table_insert(&context.types, enumeration.name_id, child_node_id); break;
			}
			case ID_AST_TRAIT: {
				a_trait trait = LOOKUP(child_node_id, a_trait);
				symbol_table_insert(&context.traits, trait.name_id, child_node_id); break;
			}
			case ID_AST_IMPORT: {
				a_import import = LOOKUP(child_node_id, a_import);
				symbol_table_insert(&context.imports, import.name_id, import.module_id); break;
			}
			case ID_AST_IMPL: break;
			default:
				FATAL("Invalid AST type: {s}", id_type_to_string(child_node_id.type));
		}
	}
}

void context_exit_module(a_module module) {
	unsigned int declarations = 0, types = 0, traits = 0;

	// reverse order so that symbol_table_remove can pop arena elements
	for (ssize_t i = module.members.size - 1; i >= 0; --i) {
		ID child_node_id = ARENA_GET(module.members, i, ID);
		switch (child_node_id.type) {
			case ID_AST_DECLARATION: {
				a_declaration declaration = LOOKUP(child_node_id, a_declaration);
				symbol_table_remove(&context.declarations, declaration.name_id); break;
			}
			case ID_AST_FUNCTION: {
				a_function function = LOOKUP(child_node_id, a_function);
				symbol_table_remove(&context.declarations, function.name_id); break;
			}
			case ID_AST_STRUCT: {
				a_structure structure = LOOKUP(child_node_id, a_structure);
				symbol_table_remove(&context.types, structure.name_id); break;
			}
			case ID_AST_ENUM: {
				a_enumeration enumeration = LOOKUP(child_node_id, a_enumeration);
				symbol_table_remove(&context.types, enumeration.name_id); break;
			}
			case ID_AST_TRAIT: {
				a_trait trait = LOOKUP(child_node_id, a_trait);
				symbol_table_remove(&context.traits, trait.name_id); break;
			}
			case ID_AST_IMPORT: {
				a_import import = LOOKUP(child_node_id, a_import);
				symbol_table_remove(&context.imports, import.name_id); break;
			}
			case ID_AST_IMPL: break;
			default:
				FATAL("Invalid AST type: {s}", id_type_to_string(child_node_id.type));
		}
	}
}

void context_enter_function(a_function function) {
	for (size_t i = 0; i < function.templates.size; ++i) {
		ID child_node_id = ARENA_GET(function.templates, i, ID);
		ASSERT1(ID_IS(child_node_id, ID_AST_SYMBOL)); // should be handled in the parser
		a_symbol symbol = LOOKUP(child_node_id, a_symbol);
		ASSERT1(symbol.name_ids.size == 1); // should be handled in the parser

		symbol_table_insert(&context.types, ARENA_GET(symbol.name_ids, 0, ID), child_node_id);
		println("Template type({i}): {s}", i, symbol_expand_path(symbol));
	}

	ASSERT1(ID_IS(function.arguments_id, ID_AST_EXPR));
	a_expression arguments = LOOKUP(function.arguments_id, a_expression);

	for (size_t i = 0; i < arguments.children.size; ++i) {
		ID arg_id = ARENA_GET(arguments.children, i, ID);
		ASSERT1(ID_IS(arg_id, ID_AST_SYMBOL)); // Should be handled in the parser

		a_symbol symbol = LOOKUP(arg_id, a_symbol);
		ASSERT1(!ID_IS_INVALID(symbol.node_id)); // Should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser

		symbol_table_insert(&context.declarations, ARENA_GET(symbol.name_ids, 0, ID), symbol.node_id);
	}

}

void context_exit_function(a_function function) {
}

void context_enter_scope(a_scope scope) {
}

void context_exit_scope(a_scope scope) {
}
