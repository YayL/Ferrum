#pragma once

#include "parser/AST.h"
#include "tables/symbol_table.h"

typedef struct context {
	struct symbol_table declarations;	// variables, function
	struct symbol_table types;			// structs, enum
	struct symbol_table traits;			// traits
	struct symbol_table imports;		// module aliases
} Context;

void context_init();

/* Add module declared variables, functions and imports */
void context_enter_module(a_module module);
void context_exit_module(a_module module);

/* Add function template types as types */
void context_enter_function(a_function function);
void context_exit_function(a_function function);

/* Add scope declared variables */
void context_enter_scope(a_scope scope);
void context_exit_scope(a_scope scope);
