#pragma once

#include "common/ID.h"

struct symbol_table {
	khash_t(map_id_to_id) map;
};

typedef struct symbol_table_entry {
	ID symbol_id; // ID of this entry
	ID name_id; // Symbol table key; interner ID
	ID node_id; // Symbol table value; symbol_registry ID
	ID shadowed_symbol_id; // Shadowed entry
} symbol_table_entry;

#include "tables/registry_manager.h"

#define INVALID_SYMBOL_ID (0)
#define SYMBOL_ID_TO_INDEX(index) ((index) - 1)

struct symbol_table symbol_table_init();

void symbol_table_insert(struct symbol_table * symbol_table, ID name_id, ID node_id);
void symbol_table_remove(struct symbol_table * symbol_table, ID name_id);

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, ID symbol_id);
struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, ID name_id);
