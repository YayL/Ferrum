#include "checker/pre_checker.h"

#include "tables/registry_manager.h"
#include "checker/symbol.h"

void resolve_impl(a_implementation * impl) {
	ASSERT1(ID_IS(impl->info.node_id, ID_AST_IMPL));

	a_symbol trait_symbol = LOOKUP(impl->trait_symbol_id, a_symbol);
	ID trait_id = qualify_symbol(trait_symbol, ID_AST_TRAIT);
	ASSERT1(ID_IS(trait_id, ID_AST_TRAIT));

	a_trait * trait = lookup(trait_id);

	// println("Trait: {s}", interner_lookup_str(trait->name_id)._ptr);
	ARENA_APPEND(&trait->implementations, impl->info.node_id);
}

void resolve_impls() {
	const struct registry_manager manager = registry_manager_get();

	size_t i = 0;
	for (size_t block = 0; block < manager.a_implementation.entries.block_count; ++block) {
		for (size_t bi = 0; bi < manager.a_implementation.entries.block_max_item_count && i < manager.a_implementation.entries.item_count; ++bi) {
			resolve_impl(block_arena_get_ref(manager.a_implementation.entries, i));
			i += 1;
		}
	}
}

void resolve_variable(a_variable * variable) {
	ASSERT1(ID_IS(variable->info.node_id, ID_AST_VARIABLE));

	// Symbol_T * place_
	// variable->type_id
}

void resolve_variables(a_root * root) {
	const struct registry_manager manager = registry_manager_get();

	size_t i = 0;
	for (size_t block = 0; block < manager.a_variable.entries.block_count; ++block) {
		for (size_t bi = 0; bi < manager.a_variable.entries.block_max_item_count && i < manager.a_variable.entries.item_count; ++bi) {
			resolve_variable(block_arena_get_ref(manager.a_variable.entries, i));
			i += 1;
		}
	}
}

void pre_checker(a_root * root) {
	resolve_impls();
	resolve_variables(root);
}
