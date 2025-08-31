#include "tables/symbol_table.h"

void symbol_table_append(struct symbol_table * symbol_table, ID ast_id);

struct symbol_table symbol_table_init() {
	return (struct symbol_table) { 
		.map = kh_init(map_id_to_id),			// store name_id -> symbol_id
	};
}

void symbol_table_insert(struct symbol_table * symbol_table, ID name_id, ID node_id) {
	struct symbol_table_entry * entry = symbol_allocate();
	entry->name_id = name_id;
	entry->node_id = node_id;

	int ret_code;
	khint_t k = kh_put(map_id_to_id, &symbol_table->map, name_id, &ret_code);

	if (ret_code == KEY_ALREADY_PRESENT) {
		entry->shadowed_symbol_id = kh_value(&symbol_table->map, k);
	}

	ASSERT(ret_code == 1, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);
	kh_value(&symbol_table->map, k) = entry->symbol_id;
}

void symbol_table_remove(struct symbol_table * symbol_table, ID name_id) {
	khint_t k = kh_get(map_id_to_id, &symbol_table->map, name_id);
	ASSERT1(k != kh_end(&symbol_table->map)); // Unable to find name_id

	ID symbol_id = kh_value(&symbol_table->map, k);
	ASSERT1(ID_IS(symbol_id, ID_SYMBOL));

	struct symbol_table_entry entry = LOOKUP(symbol_id, struct symbol_table_entry);
	ASSERT1(id_is_equal(entry.symbol_id, symbol_id));

	if (!ID_IS_INVALID(entry.shadowed_symbol_id)) {
		kh_value(&symbol_table->map, k) = entry.shadowed_symbol_id;
	} else {
		kh_del(map_id_to_id, &symbol_table->map, k);
	}

	_symbol_remove_only_last_allowed(symbol_id);
}

ID symbol_table_name_to_id(const struct symbol_table * symbol_table, ID name_id) {
	unsigned int k = kh_get(map_id_to_id, &symbol_table->map, name_id);

	if (k == kh_end(&symbol_table->map)) {
		return INVALID_ID;
	}

	return kh_value(&symbol_table->map, k);
}

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, ID symbol_id) {
	return LOOKUP(symbol_id, struct symbol_table_entry);
}

struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, ID name_id) {
	ID id = symbol_table_name_to_id(symbol_table, name_id);

	if (ID_IS_INVALID(id)) {
		return (struct symbol_table_entry) { .symbol_id = INVALID_ID };
	}

	return symbol_table_get_by_id(symbol_table, id);
}
