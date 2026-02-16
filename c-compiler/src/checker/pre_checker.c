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

		// println("{s}", ast_to_string(impl_symbol.info.node_id));

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

void index_implicit_casts() {
	a_trait imp_cast_trait = LOOKUP(context_get_implicit_cast_trait(), a_trait);

	a_implementation * identity_impl = ast_allocate(ID_AST_IMPL, INVALID_ID);

	identity_impl->trait_symbol_id = imp_cast_trait.info.node_id;

	a_symbol * identity_impl_generic_template_symbol = ast_allocate(ID_AST_SYMBOL, identity_impl->info.node_id);
	identity_impl_generic_template_symbol->name_ids.size = 0;
	identity_impl_generic_template_symbol->name_id = interner_intern(source_span_init_from_string(STRING_FROM_LITERAL("T")));

	Symbol_T * identity_impl_generic_template_type = type_allocate(ID_SYMBOL_TYPE);
	identity_impl_generic_template_type->symbol_id = identity_impl_generic_template_symbol->info.node_id;

	ARENA_APPEND(&identity_impl->generics, identity_impl_generic_template_symbol->info.node_id);

	a_symbol * identity_type_from_symbol = ast_allocate(ID_AST_SYMBOL, identity_impl->info.node_id);
	a_variable * identity_type_from_variable = ast_allocate(ID_AST_VARIABLE, identity_impl->info.node_id);

	identity_type_from_variable->name_id = interner_intern(source_span_init_from_string(STRING_FROM_LITERAL("From"))); 
	identity_type_from_variable->type_id = identity_impl_generic_template_type->info.type_id;

	identity_type_from_symbol->name_id = identity_type_from_variable->name_id;
	identity_type_from_symbol->name_ids.size = 0;
	identity_type_from_symbol->node_id = identity_type_from_variable->info.node_id;

	a_symbol * identity_type_to_symbol = ast_allocate(ID_AST_SYMBOL, identity_impl->info.node_id);
	a_variable * identity_type_to_variable = ast_allocate(ID_AST_VARIABLE, identity_impl->info.node_id);

	identity_type_to_variable->name_id = interner_intern(source_span_init_from_string(STRING_FROM_LITERAL("To")));
	identity_type_to_variable->type_id = identity_impl_generic_template_type->info.type_id;

	identity_type_to_symbol->name_id = identity_type_to_variable->name_id;
	identity_type_to_symbol->name_ids.size = 0;
	identity_type_to_symbol->node_id = identity_type_to_variable->info.node_id;

	arena_grow(&identity_impl->templates, 2);
	ARENA_APPEND(&identity_impl->templates, identity_type_from_symbol->info.node_id);
	ARENA_APPEND(&identity_impl->templates, identity_type_to_symbol->info.node_id);

	uint32_t sorted[imp_cast_trait.implementations.size];
	memset(sorted, 0, sizeof(sorted));

	// Setup initial list of candidates
	for (size_t i = 0; i < imp_cast_trait.implementations.size; ++i) {
		if (sorted[i]) {
			continue;
		}

		sorted[i] = 1;

		a_implementation impl1 = LOOKUP(ARENA_GET(imp_cast_trait.implementations, i, ID), a_implementation);
		ID from_type_id1 = ast_get_type_of(ARENA_GET(impl1.templates, 0, ID));

		Arena candidates = arena_init(sizeof(ID));
		
		ASSERT1(impl1.generics.size == 1);

		ARENA_APPEND(&candidates, identity_impl->info.node_id);
		ARENA_APPEND(&candidates, impl1.info.node_id);

		// Retrieve all implicit casts with same From type
		for (size_t j = i + 1; j < imp_cast_trait.implementations.size; ++j) {
			if (sorted[j]) {
				continue;
			}

			a_implementation impl2 = LOOKUP(ARENA_GET(imp_cast_trait.implementations, j, ID), a_implementation);
			ID from_type_id2 = ast_get_type_of(ARENA_GET(impl2.templates, 0, ID));

			if (type_check_equal(from_type_id1, from_type_id2)) {
				ARENA_APPEND(&candidates, impl2.info.node_id);
				sorted[j] = 1;
			}
		}

		context_add_implicit_casts(from_type_id1, candidates);
	}
}

void pre_checker(a_root * root) {
	resolve_impls();
	context_init(find_implicit_cast_trait());
	index_implicit_casts();

	// LOOP_OVER_REGISTRY(a_symbol, symbol, {
	// 	if (ID_IS(symbol->node_id, ID_PLACE_TYPE)) {
	// 		println("{s}", ast_to_string(symbol->info.node_id));
	// 		print_ast_tree(symbol->info.scope_id);
	// 	}
	// });
}
