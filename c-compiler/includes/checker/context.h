#pragma once

#include "codegen/AST.h"
#include "tables/symbol.h"

struct context {
	struct symbol_table declarations;
	struct symbol_table types;
};

/* Add module declared variables, functions and imports */
void context_enter_module(struct symbol_table * symbol_table, struct AST * module_ast);
void context_exit_module(struct symbol_table * symbol_table, struct AST * module_ast);

/* Add function template types as types */
void context_enter_function(struct symbol_table * symbol_table, struct AST * function_ast);
void context_exit_function(struct symbol_table * symbol_table, struct AST * function_ast);

/* Add scope declared variables */
void context_enter_scope(struct symbol_table * symbol_table, struct AST * scope);
void context_exit_scope(struct symbol_table * symbol_table, struct AST * scope);
