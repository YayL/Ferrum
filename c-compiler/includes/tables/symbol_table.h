#pragma once

#include "common/data/symbolmap.h"
#include "common/ID.h"

struct symbol_table {
	struct symbol_map declarations; // variables, function
	struct symbol_map types;		// structs, enums
	struct symbol_map traits;		// traits
	struct symbol_map imports;		// module symbols
};

struct symbol_table symbol_table_init();

void symbol_table_insert(struct symbol_table * table, ID value);
void symbol_table_remove(struct symbol_table * table, ID value);

void symbol_table_extend(struct symbol_table * table, const struct symbol_table other);
void symbol_table_clear(struct symbol_table * table);
