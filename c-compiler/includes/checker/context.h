#pragma once

#include "parser/AST.h"
#include "tables/symbol_table.h"
#include "parser/types.h"

KHASH_INIT(map_type_id_to_arena, ID, Arena, 1, type_id_to_hash, type_check_equal);

typedef struct context {
	struct symbol_table symbol_table;
	ID implicit_cast_trait;
	khash_t(map_type_id_to_arena) implicit_casts;
} Context;

void context_init(ID implicit_cast_trait);

ID context_get_implicit_cast_trait();
khash_t(map_type_id_to_arena) * context_get_implicit_casts();

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

Arena * context_find_implicit_casts(ID type_id);
void context_add_implicit_cast(ID type_id, ID implicit_cast);
void context_add_implicit_casts(ID type_id, Arena arena);
