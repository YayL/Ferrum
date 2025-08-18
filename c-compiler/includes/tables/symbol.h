#pragma once
#include "common/arena.h"
#include "common/hashmap.h"

#define SYMBOL_HM_VALUE_TYE unsigned int
KHASH_MAP_INIT_INT(symbol_hm, SYMBOL_HM_VALUE_TYE);

struct symbol_table {
	Arena entries;
	khash_t(symbol_hm) map;
};

struct symbol_table_entry {
	struct AST * ast;
	unsigned int name_id;
	unsigned int shadowed_symbol_id;

	unsigned int symbol_id;
};

#define INVALID_SYMBOL_ID (0)
#define SYMBOL_ID_TO_INDEX(index) ((index) - 1)

struct symbol_table symbol_table_init();

/* Add module declared variables, functions and imports */
void symbol_table_enter_module(struct symbol_table * symbol_table, struct AST * module_ast);
void symbol_table_exit_module(struct symbol_table * symbol_table, struct AST * module_ast);

/* Add function template types as types */
void symbol_table_enter_function(struct symbol_table * symbol_table, struct AST * function_ast);
void symbol_table_exit_function(struct symbol_table * symbol_table, struct AST * function_ast);

/* Add scope declared variables */
void symbol_table_enter_scope(struct symbol_table * symbol_table, struct AST * scope);
void symbol_table_exit_scope(struct symbol_table * symbol_table, struct AST * scope);

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, unsigned int symbol_id);
struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, unsigned int name_id);
