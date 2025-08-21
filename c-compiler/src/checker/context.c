#include "checker/context.h"

Context context;

void context_enter_module(struct AST * module_ast) {
	ASSERT1(module_ast->type == AST_MODULE);
	a_module module = module_ast->value.module;
}

void context_exit_module(struct AST * module_ast) {
	ASSERT1(module_ast->type == AST_MODULE);
	a_module module = module_ast->value.module;
}

void context_enter_function(struct AST * function_ast) {
	ASSERT1(function_ast->type == AST_FUNCTION);
	a_function function = function_ast->value.function;
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
