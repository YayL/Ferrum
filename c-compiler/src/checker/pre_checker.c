#include "checker/pre_checker.h"

#include "tables/registry_manager.h"
#include "checker/symbol.h"
#include "checker/context.h"

void resolve_impl(a_implementation * impl) {
	ASSERT1(ID_IS(impl->info.node_id, ID_AST_IMPL));

	a_symbol * trait_symbol = lookup(impl->trait_symbol_id);
	ID trait_id = qualify_symbol(trait_symbol, ID_AST_TRAIT);
	ASSERT(ID_IS(trait_id, ID_AST_TRAIT), "Unable to find trait");

	a_trait * trait = lookup(trait_id);

	// println("Trait: {s}", interner_lookup_str(trait->name_id)._ptr);
	ARENA_APPEND(&trait->implementations, impl->info.node_id);

	if (impl->templates.size != trait->templates.size) {
		ERROR("Implementation does not declare all template types");
		print_trace();
		exit(1);
	}

	for (size_t i = 0; i < impl->templates.size; ++i) {
		a_symbol impl_symbol = LOOKUP(ARENA_GET(impl->templates, i, ID), a_symbol);
		a_symbol trait_symbol = LOOKUP(ARENA_GET(trait->templates, i, ID), a_symbol);

		if (!id_is_equal(impl_symbol.name_id, trait_symbol.name_id)) {
			ERROR("Implementation does not match trait template type order");
			print_trace();
			exit(1);
		}

		ASSERT1(ID_IS(impl_symbol.node_id, ID_AST_VARIABLE));
		ID type_id = LOOKUP(impl_symbol.node_id, a_variable).type_id;
		if (ID_IS_INVALID(type_id)) {
			ERROR("Implementation does not specify template type \"{s}\"", interner_lookup_str(trait_symbol.name_id)._ptr);
			print_trace();
			exit(1);
		}
	}
}

void resolve_impls() {
	LOOP_OVER_REGISTRY(a_implementation, item, {
		resolve_impl(item);
	});
}

ID find_implicit_cast_trait() {
	ID implicit_cast_trait = interner_intern(SOURCE_SPAN_INIT("#ImplicitCast"));
	LOOP_OVER_REGISTRY(a_trait, item, {
		if (id_is_equal(item->name_id, implicit_cast_trait)) {
			return item->info.node_id;
		}
	});

	FATAL("Unable to find \"#ImplicitCast\" trait");
}

void pre_checker(a_root * root) {
	resolve_impls();
	context_init(find_implicit_cast_trait());

	// LOOP_OVER_REGISTRY(a_symbol, symbol, {
	// 	if (ID_IS(symbol->node_id, ID_PLACE_TYPE)) {
	// 		println("{s}", ast_to_string(symbol->info.node_id));
	// 		print_ast_tree(symbol->info.scope_id);
	// 	}
	// });
}
