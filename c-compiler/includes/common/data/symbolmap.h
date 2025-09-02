#pragma once

#include "common/ID.h"

struct symbol_map {
	khash_t(map_id_to_id) map; // name_id -> symbol_id
};

typedef struct symbol_map_entry {
	ID symbol_id;			// ID of this entry
	ID name_id;				// Symbol table key; interner ID
	ID node_id;				// Symbol table value; symbol_registry ID
	ID shadowed_symbol_id;	// Shadowed entry
} symbol_map_entry;

struct symbol_map symbol_map_init();

void symbol_map_insert(struct symbol_map * symbol_map, ID name_id, ID node_id);
void symbol_map_remove(struct symbol_map * symbol_map, ID name_id);
void symbol_map_extend(struct symbol_map * dest, const struct symbol_map * src);
void symbol_map_clear(struct symbol_map * symbol_map);

struct symbol_map_entry symbol_map_get_by_id(const struct symbol_map * symbol_map, ID symbol_id);
struct symbol_map_entry symbol_map_get_by_name(const struct symbol_map * symbol_map, ID name_id);
