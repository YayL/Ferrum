#include "tables/registry_manager.h"

struct registry_manager manager;

void registry_manager_setup_instance() {
	manager = registry_manager_init();
}

const struct registry_manager registry_manager_get() {
	return manager;
}

struct registry_manager * _registry_manager_get_ref() {
	return &manager;
}

struct symbol_map_entry * symbol_allocate() {
	return registry_manager_allocate(&manager, ID_SYMBOL);
}

struct interner_entry * interner_allocate() {
	return registry_manager_allocate(&manager, ID_INTERNER);
}

void * tc_allocate(enum id_type type) {
	return registry_manager_allocate(&manager, type);
}

void * ast_allocate(enum id_type type, ID scope_id) {
	void * ref = registry_manager_allocate(&manager, type);
	((struct AST_info *) ref)->scope_id = scope_id;
	return ref;
}

void * type_allocate(enum id_type type) {
	return registry_manager_allocate(&manager, type);
}

struct symbol_table_entry * symbol_lookup(ID symbol_id) {
	ASSERT1(ID_IS(symbol_id, ID_SYMBOL));
	return registry_manager_lookup(&manager, symbol_id);
}

struct interner_entry * interner_lookup(ID interner_id) {
	ASSERT1(ID_IS(interner_id, ID_INTERNER));
	return registry_manager_lookup(&manager, interner_id);
}

void * lookup(ID id) {
	return registry_manager_lookup(&manager, id);
}

void _symbol_remove_only_last_allowed(ID symbol_id) {
	registry_manager_remove(&manager, symbol_id);
}
