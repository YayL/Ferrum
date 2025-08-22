#pragma once

#include "codegen/AST.h"
#include "tables/symbol_table.h"

typedef struct context {
	struct symbol_table declarations;	// variables, function
	struct symbol_table types;			// structs, enum
	struct symbol_table traits;			// traits
	struct symbol_table imports;		// module aliases
} Context;

void context_init();

/* Add module declared variables, functions and imports */
void context_enter_module(struct AST * module_ast);
void context_exit_module(struct AST * module_ast);

/* Add function template types as types */
void context_enter_function(struct AST * function_ast);
void context_exit_function(struct AST * function_ast);

/* Add scope declared variables */
void context_enter_scope(struct AST * scope);
void context_exit_scope(struct AST * scope);
