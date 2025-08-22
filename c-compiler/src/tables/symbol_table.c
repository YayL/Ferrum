#include "tables/symbol_table.h"
#include "tables/registry.h"

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item);

struct symbol_table symbol_table_init() {
	return (struct symbol_table) {
		.symbol_registry = registry_init(sizeof(struct AST *)),
	};
}

void symbol_table_insert(struct symbol_table * symbol_table, ID name_id, struct AST * node) {
	struct symbol_table_entry entry = {
		.name_id = name_id,
		.ast = node,
	};

	unsigned int previous_value = INVALID_REGISTRY_ID;
	struct symbol_table_entry * addr = registry_insert(&symbol_table->symbol_registry, name_id, &previous_value);

	if (previous_value != INVALID_REGISTRY_ID) {
		entry.shadowed_symbol_id = previous_value;
	}

	entry.symbol_id = symbol_table->symbol_registry.entries.size; // This has already been incremented by 1 in registry_insert
	*addr = entry;
}

void symbol_table_remove(struct symbol_table * symbol_table, unsigned int name_id) {
	khint_t k = kh_get(map_id_to_id, &symbol_table->symbol_registry.map, name_id);
	ASSERT1(k != kh_end(&symbol_table->symbol_registry.map)); // Unable to find name_id

	unsigned int symbol_id = kh_value(&symbol_table->symbol_registry.map, k);
	// Subtract by one because symbol ids starts at 1 and their index is one less
	struct symbol_table_entry entry = ARENA_GET(symbol_table->symbol_registry.entries, symbol_id - 1, struct symbol_table_entry);
	ASSERT1(entry.symbol_id == symbol_id);

	if (entry.shadowed_symbol_id != 0) {
		kh_value(&symbol_table->symbol_registry.map, k) = entry.shadowed_symbol_id;
	} else {
		kh_del(map_id_to_id, &symbol_table->symbol_registry.map, name_id);
	}

	ASSERT1(entry.symbol_id == symbol_table->symbol_registry.entries.size); // A promise made when calling symbol_table_remove
	arena_shrink(&symbol_table->symbol_registry.entries, 1);
}

struct symbol_table_entry symbol_table_entry_init(const unsigned int ID, struct AST * item) {
	return (struct symbol_table_entry) {
		.ast = item,
		.symbol_id = ID,
	};
}

unsigned int symbol_table_name_to_id(const struct symbol_table * symbol_table, unsigned int name_id) {
	unsigned int k = kh_get(map_id_to_id, &symbol_table->symbol_registry.map, name_id);

	if (k == kh_end(&symbol_table->symbol_registry.map)) {
		return INVALID_SYMBOL_ID;
	}

	return kh_value(&symbol_table->symbol_registry.map, k);
}

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, unsigned int symbol_id) {
	return *(struct symbol_table_entry *) arena_get_ref(symbol_table->symbol_registry.entries, symbol_id);
}

struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, unsigned int name_id) {
	unsigned int ID = symbol_table_name_to_id(symbol_table, name_id);

	if (ID == INVALID_SYMBOL_ID) {
		return (struct symbol_table_entry) {.symbol_id = INVALID_SYMBOL_ID};
	}

	return symbol_table_get_by_id(symbol_table, ID);
}

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item) {
	struct symbol_table_entry entry = symbol_table_entry_init(symbol_table->symbol_registry.entries.size, item);
	ARENA_APPEND(&symbol_table->symbol_registry.entries, entry);
}
