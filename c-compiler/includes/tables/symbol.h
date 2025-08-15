#pragma once
#include "common/arena.h"

struct symbol_table {
	Arena entries;
};

struct symbol_table_entry {
	struct AST * ast;

	const char * name;
	unsigned int symbol_ID;
};

#define INVALID_SYMBOL_ID (-1)

struct symbol_table symbol_table_init();

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item);

unsigned int symbol_table_name_to_id(const struct symbol_table symbol_table, const char * name);
struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table symbol_table, unsigned int symbol_ID);
struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table symbol_table, const char * name);
