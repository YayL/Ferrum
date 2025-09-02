#pragma once

#include "parser/AST.h"
#include "tables/symbol_table.h"

typedef struct context {
	struct symbol_table symbol_table;
} Context;

void context_init();

Arena context_lookup_all_declarations(ID name_id);
ID context_lookup_declaration(ID name_id);

/* Add module declared variables, functions and imports */
void context_enter_module(a_module module);
void context_exit_module(a_module module);

/* Add function template types as types */
void context_enter_function(a_function function);
void context_exit_function(a_function function);

/* Add scope declared variables */
void context_enter_scope(a_scope scope);
void context_exit_scope(a_scope scope);
