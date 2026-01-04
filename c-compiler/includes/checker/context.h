#pragma once

#include "parser/AST.h"
#include "tables/symbol_table.h"

typedef struct context {
	struct symbol_table symbol_table;
	ID implicit_cast_trait;
} Context;

void context_init(ID implicit_cast_trait);

ID context_get_implicit_cast_trait();

Arena context_lookup_all_declarations(ID name_id);
ID context_lookup_type(ID name_id);
ID context_lookup_declaration(ID name_id);

/* Add module declared variables, functions and imports */
void context_enter_module(a_module module);
void context_exit_module(a_module module);

void context_add_template_list(Arena arena);
void context_remove_template_list(Arena arena);

void context_add_declaration_list(Arena arena);
void context_remove_declaration_list(Arena arena);
