#pragma once

#include "tables/registry.h"
#include "codegen/AST.h"

struct symbol_table {
	struct registry symbol_registry;
};

struct symbol_table_entry {
	struct AST * ast;
	ID name_id;
	ID shadowed_symbol_id;

	ID symbol_id;
};

#define INVALID_SYMBOL_ID (0)
#define SYMBOL_ID_TO_INDEX(index) ((index) - 1)

struct symbol_table symbol_table_init();

void symbol_table_insert(struct symbol_table * symbol_table, ID id, struct AST * node);
void symbol_table_remove(struct symbol_table * symbol_table, ID name_id);

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, ID symbol_id);
struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, ID name_id);
