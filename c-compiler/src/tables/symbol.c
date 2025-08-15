#include "tables/symbol.h"

#include "common/arena.h"
#include "codegen/AST.h"
#include "common/logger.h"

struct symbol_table symbol_table_init() {
	return (struct symbol_table) { 
		.entries = arena_init(sizeof(struct symbol_table_entry))
	};
}

struct symbol_table_entry symbol_table_entry_init(const unsigned int ID, struct AST * item) {
	return (struct symbol_table_entry) {
		.ast = item,
		.symbol_ID = ID,
	};
}

void symbol_table_append(struct symbol_table * symbol_table, struct AST * item) {
	ASSERT1(symbol_table != NULL);
	ASSERT1(item != NULL);

	struct symbol_table_entry entry = symbol_table_entry_init(symbol_table->entries.size, item);
	ARENA_APPEND(&symbol_table->entries, entry);
}

unsigned int symbol_table_name_to_id(const struct symbol_table symbol_table, const char * name) {

}

struct symbol_table_entry symbol_table_get_by_id(const struct symbol_table symbol_table, unsigned int symbol_ID) {
	return *(struct symbol_table_entry *) arena_get(symbol_table.entries, symbol_ID);
}

struct symbol_table_entry symbol_table_get_by_name(const struct symbol_table symbol_table, const char * name) {
	unsigned int ID = symbol_table_name_to_id(symbol_table, name);

	if (ID == INVALID_SYMBOL_ID) {
		return (struct symbol_table_entry) {.symbol_ID = INVALID_SYMBOL_ID};
	}

	return symbol_table_get_by_id(symbol_table, ID);
}
