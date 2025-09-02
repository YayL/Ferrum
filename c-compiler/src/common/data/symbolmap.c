#include "common/data/symbolmap.h"

#include "tables/registry_manager.h"

struct symbol_map symbol_map_init() {
	return (struct symbol_map) { 
		.map = kh_init(map_id_to_id),
	};
}

void symbol_map_insert(struct symbol_map * symbol_map, ID name_id, ID node_id) {
	struct symbol_map_entry * entry = symbol_allocate();
	entry->name_id = name_id;
	entry->node_id = node_id;
	entry->shadowed_symbol_id = INVALID_ID;

	int ret_code;
	khint_t k = kh_put(map_id_to_id, &symbol_map->map, name_id, &ret_code);

	if (ret_code == KH_PUT_ALREADY_PRESENT) {
		entry->shadowed_symbol_id = kh_value(&symbol_map->map, k);
	}

	ASSERT(ret_code == KH_PUT_SUCCESS, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);
	kh_value(&symbol_map->map, k) = entry->symbol_id;
}

void symbol_map_remove(struct symbol_map * symbol_map, ID name_id) {
	khint_t k = kh_get(map_id_to_id, &symbol_map->map, name_id);
	ASSERT1(k != kh_end(&symbol_map->map)); // Unable to find name_id

	ID symbol_id = kh_value(&symbol_map->map, k);
	ASSERT1(ID_IS(symbol_id, ID_SYMBOL));

	struct symbol_map_entry entry = LOOKUP(symbol_id, struct symbol_map_entry);
	ASSERT1(id_is_equal(entry.symbol_id, symbol_id));

	if (!ID_IS_INVALID(entry.shadowed_symbol_id)) {
		kh_value(&symbol_map->map, k) = entry.shadowed_symbol_id;
	} else {
		kh_del(map_id_to_id, &symbol_map->map, k);
	}

	_symbol_remove_only_last_allowed(symbol_id);
}

void symbol_map_extend(struct symbol_map * dest, const struct symbol_map * src) {
	ID key, symbol_id;

	kh_foreach(&src->map, key, symbol_id, {
		struct symbol_map_entry entry = symbol_map_get_by_id(src, symbol_id);

		int ret_code;
		khint_t k = kh_put(map_id_to_id, &dest->map, key, &ret_code);

		if (ret_code == KH_PUT_ALREADY_PRESENT) {
			ASSERT(ID_IS_INVALID(entry.shadowed_symbol_id), "I don't want to handle this now, it most likely requires a linked-list traversal...");
			entry.shadowed_symbol_id = kh_value(&dest->map, k);
		}

		ASSERT(ret_code == KH_PUT_SUCCESS, "Some error occured while retrieving from hashmap. Error code {i}", ret_code);
		kh_value(&dest->map, k) = entry.symbol_id;
	});
}

void symbol_map_clear(struct symbol_map * symbol_map) {
	kh_clear(map_id_to_id, &symbol_map->map);
}

ID symbol_map_name_to_id(const struct symbol_map * symbol_map, ID name_id) {
	unsigned int k = kh_get(map_id_to_id, &symbol_map->map, name_id);

	if (k == kh_end(&symbol_map->map)) {
		return INVALID_ID;
	}

	return kh_value(&symbol_map->map, k);
}

struct symbol_map_entry symbol_map_get_by_id(const struct symbol_map * symbol_map, ID symbol_id) {
	return LOOKUP(symbol_id, struct symbol_map_entry);
}

struct symbol_map_entry symbol_map_get_by_name(const struct symbol_map * symbol_map, ID name_id) {
	ID id = symbol_map_name_to_id(symbol_map, name_id);

	if (ID_IS_INVALID(id)) {
		return (struct symbol_map_entry) { .symbol_id = INVALID_ID };
	}

	return symbol_map_get_by_id(symbol_map, id);
}
