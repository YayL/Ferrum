#include "checker/pre_checker.h"

#include "tables/registry_manager.h"
#include "checker/symbol.h"
#include "checker/context.h"

#define LOOP_OVER_REGISTRY(TYPE, CODE) { \
	Registry registry = registry_manager_get().TYPE; \
	for (size_t i = 0, block = 0; block < registry.entries.block_count; ++block) { \
		for (size_t bi = 0; bi < registry.entries.block_max_item_count && i < registry.entries.item_count; ++bi, ++i) { \
			TYPE * item = block_arena_get_ref(registry.entries, i); \
			CODE \
		} \
	} \
}

void resolve_impl(a_implementation * impl) {
	ASSERT1(ID_IS(impl->info.node_id, ID_AST_IMPL));

	a_symbol * trait_symbol = lookup(impl->trait_symbol_id);
	ID trait_id = qualify_symbol(trait_symbol, ID_AST_TRAIT);
	ASSERT1(ID_IS(trait_id, ID_AST_TRAIT));

	a_trait * trait = lookup(trait_id);

	// println("Trait: {s}", interner_lookup_str(trait->name_id)._ptr);
	ARENA_APPEND(&trait->implementations, impl->info.node_id);

	if (impl->trait_templates.size != trait->templates.size) {
		ERROR("Implementation does not declare all template types");
		print_trace();
		exit(1);
	}

	for (size_t i = 0; i < impl->trait_templates.size; ++i) {
		a_symbol impl_symbol = LOOKUP(ARENA_GET(impl->trait_templates, i, ID), a_symbol);
		a_symbol trait_symbol = LOOKUP(ARENA_GET(trait->templates, i, ID), a_symbol);

		if (!id_is_equal(impl_symbol.name_id, trait_symbol.name_id)) {
			ERROR("Implementation does not match trait template type order");
			print_trace();
			exit(1);
		}

		ASSERT1(ID_IS(impl_symbol.node_id, ID_AST_VARIABLE));
		ID type_id = ARENA_GET(impl->trait_templates, i, ID) = LOOKUP(impl_symbol.node_id, a_variable).type_id;
		if (ID_IS_INVALID(type_id)) {
			ERROR("Implementation does not specify template type \"{s}\"", interner_lookup_str(trait_symbol.name_id)._ptr);
			print_trace();
			exit(1);
		}
	}
}

void resolve_impls() {
	LOOP_OVER_REGISTRY(a_implementation, {
		resolve_impl(item);
	});
}

ID find_implicit_cast_trait() {
	ID implicit_cast_trait = interner_intern(SOURCE_SPAN_INIT("#ImplicitCast"));
	LOOP_OVER_REGISTRY(a_trait, {
		if (id_is_equal(item->name_id, implicit_cast_trait)) {
			return item->info.node_id;
		}
	});

	FATAL("Unable to find \"#ImplicitCast\" trait");
}

void pre_checker(a_root * root) {
	resolve_impls();
	context_init(find_implicit_cast_trait());
}
