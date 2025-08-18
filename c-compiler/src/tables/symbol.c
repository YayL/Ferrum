#include "tables/symbol.h"

#include "common/arena.h"
#include "codegen/AST.h"
#include "common/hashmap.h"

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item);

struct symbol_table symbol_table_init() {
	return (struct symbol_table) { 
		.entries = arena_init(sizeof(struct symbol_table_entry)),
		.map = kh_init(symbol_hm),
	};
}

struct symbol_table_entry symbol_table_entry_init(const unsigned int ID, struct AST * item) {
	return (struct symbol_table_entry) {
		.ast = item,
		.symbol_id = ID,
	};
}

unsigned int symbol_table_name_to_id(const struct symbol_table * symbol_table, unsigned int name_id) {
	unsigned int k = kh_get(symbol_hm, &symbol_table->map, name_id);

	if (k == kh_end(&symbol_table->map)) {
		return INVALID_SYMBOL_ID;
	}

	return kh_value(&symbol_table->map, k);
}

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table * symbol_table, unsigned int symbol_id) {
	return *(struct symbol_table_entry *) arena_get(symbol_table->entries, symbol_id);
}

struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table * symbol_table, unsigned int name_id) {
	unsigned int ID = symbol_table_name_to_id(symbol_table, name_id);

	if (ID == INVALID_SYMBOL_ID) {
		return (struct symbol_table_entry) {.symbol_id = INVALID_SYMBOL_ID};
	}

	return symbol_table_get_by_id(symbol_table, ID);
}

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item) {
	struct symbol_table_entry entry = symbol_table_entry_init(symbol_table->entries.size, item);
	ARENA_APPEND(&symbol_table->entries, entry);
}
