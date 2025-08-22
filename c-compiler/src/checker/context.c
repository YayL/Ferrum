#include "checker/context.h"

#include "checker/symbols.h"
#include "common/arena.h"
#include "common/logger.h"

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

void context_enter_module(struct AST * module_ast) {
	ASSERT1(module_ast->type == AST_MODULE);
	a_module module = module_ast->value.module;
	for (size_t i = 0; i < module.members.size; ++i) {
		struct AST * node = ARENA_GET(module.members, i, struct AST *);
		unsigned int name_id;
		struct symbol_table * sym_table;

		switch (node->type) {
			case AST_DECLARATION:
				symbol_table_insert(&context.declarations, node->value.declaration.name_id, node); break;
			case AST_FUNCTION:
				symbol_table_insert(&context.declarations, node->value.function.name_id, node); break;
			case AST_STRUCT:
				symbol_table_insert(&context.types, node->value.structure.name_id, node); break;
			case AST_ENUM:
				symbol_table_insert(&context.types, node->value.enumeration.name_id, node); break;
			case AST_TRAIT:
				symbol_table_insert(&context.traits, node->value.trait.name_id, node); break;
			case AST_IMPORT:
				symbol_table_insert(&context.imports, node->value.import.name_id, node->value.import.module); break;
			case AST_IMPL: break;
			default:
				FATAL("Invalid AST type: {s}", ast_type_to_str(node->type));
		}
	}
}

void context_exit_module(struct AST * module_ast) {
	ASSERT1(module_ast->type == AST_MODULE);
	a_module module = module_ast->value.module;
	unsigned int declarations = 0, types = 0, traits = 0;

	// reverse order so that symbol_table_remove can pop arena elements
	for (ssize_t i = module.members.size - 1; i >= 0; --i) {
		struct AST * node = ARENA_GET(module.members, i, struct AST *);
		switch (node->type) {
			case AST_DECLARATION:
				symbol_table_remove(&context.declarations, node->value.declaration.name_id); break;
			case AST_FUNCTION:
				symbol_table_remove(&context.declarations, node->value.function.name_id); break;
			case AST_STRUCT:
				symbol_table_remove(&context.types, node->value.structure.name_id); break;
			case AST_ENUM:
				symbol_table_remove(&context.types, node->value.enumeration.name_id); break;
			case AST_TRAIT:
				symbol_table_remove(&context.traits, node->value.trait.name_id); break;
			case AST_IMPORT:
				symbol_table_remove(&context.imports, node->value.import.name_id); break;
			case AST_IMPL: break;
			default:
				FATAL("Invalid AST type: {s}", ast_type_to_str(node->type));
		}
	}
}

void context_enter_function(struct AST * function_ast) {
	ASSERT1(function_ast->type == AST_FUNCTION);
	a_function function = function_ast->value.function;

	for (size_t i = 0; i < function.templates.size; ++i) {
		struct AST * node = ARENA_GET(function.templates, i, struct AST *);
		ASSERT1(node->type == AST_SYMBOL); // should be handled in the parser
		a_symbol symbol = node->value.symbol;
		ASSERT1(symbol.name_ids.size == 1); // should be handled in the parser

		symbol_table_insert(&context.types, ARENA_GET(symbol.name_ids, 0, unsigned int), node);
		println("Template type({i}): {s}", i, symbol_expand_path(node));
	}

	ASSERT1(function.arguments->type == AST_EXPR);
	a_expression arguments = function.arguments->value.expression;
	for (size_t i = 0; i < arguments.children.size; ++i) {
		struct AST * arg = ARENA_GET(arguments.children, i, struct AST *);
		ASSERT1(arg->type == AST_SYMBOL); // Should be handled in the parser

		a_symbol symbol = arg->value.symbol;
		ASSERT1(symbol.node != NULL); // Should be handled in the parser
		ASSERT1(symbol.name_ids.size == 1); // Should be handled in the parser

		symbol_table_insert(&context.declarations, ARENA_GET(symbol.name_ids, 0, unsigned int), symbol.node);
	}

}

void context_exit_function(struct AST * function_ast) {
	ASSERT1(function_ast->type == AST_FUNCTION);
	a_function function = function_ast->value.function;
}

void context_enter_scope(struct AST * scope_ast) {
	ASSERT1(scope_ast->type == AST_SCOPE);
	a_scope scope = scope_ast->value.scope;
}

void context_exit_scope(struct AST * scope_ast) {
	ASSERT1(scope_ast->type == AST_SCOPE);
	a_scope scope = scope_ast->value.scope;
}
