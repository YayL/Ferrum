#include "checker/typing.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"
#include "checker/symbol.h"
#include "checker/context.h"

char type_has_trait_implementation(ID type, ID trait);
char check_template_def_matches_type(ID template_id, ID type_id);
ID get_template_from_templates(a_symbol symbol, khash_t(map_id_to_id) * templates, khint_t * found);

char is_equal_types_and_template_resolution(ID caller_type, ID func_type, khash_t(map_id_to_id) * caller_templates, khash_t(map_id_to_id) * func_templates, unsigned int * specificity_cost) {
	// println("Check: {2s: == }", type_to_str(caller_type), type_to_str(func_type));

	// Auto de-place
	if (ID_IS(caller_type, ID_PLACE_TYPE) && !ID_IS(func_type, ID_PLACE_TYPE) && !ID_IS(func_type, ID_SYMBOL_TYPE)) {
		Place_T caller_place = LOOKUP(caller_type, Place_T);
		caller_type = caller_place.basetype_id;
	}

	switch (func_type.type) {
		case ID_NUMERIC_TYPE: {
			if (!ID_IS(caller_type, ID_NUMERIC_TYPE)) {
				return 0;
			}
			
			Numeric_T caller = LOOKUP(caller_type, Numeric_T), func = LOOKUP(func_type, Numeric_T);
			return caller.type == func.type && caller.width == func.width;
		} break;
		case ID_SYMBOL_TYPE: {
			Symbol_T func = LOOKUP(func_type, Symbol_T);
			a_symbol * func_sym = lookup(func.symbol_id);

			if (func_templates != NULL) {
				khint_t found = 0; // the kh_value key of the templates lookup
				ID template_type_id = get_template_from_templates(*func_sym, func_templates, &found);

				// If func_sym is a template (this is not equivalent to ID_IS_INVALID(template_type_id)
				// because templates is populated with empty values at the start)
				if (found != kh_end(func_templates)) {
					if (ID_IS_INVALID(template_type_id)) {
						// Template type was not known
						if (ID_IS(caller_type, ID_PLACE_TYPE)) {
							caller_type = LOOKUP(caller_type, Place_T).basetype_id;
							ASSERT1(!ID_IS_INVALID(caller_type));
						}

						kh_value(func_templates, found) = template_type_id = caller_type;
						// println("{s} <- {s}", type_to_str(func_type), type_to_str(caller_type));
						return 1;
					}

					// Template type already known
					if (ID_IS(template_type_id, ID_AST_VARIABLE)) {
						a_variable var = LOOKUP(template_type_id, a_variable);
						template_type_id = var.type_id;
					}

					// println("{s} -> {s}", type_to_str(func_type), type_to_str(template_type_id)); 
					return is_valid_equal_type(caller_type, template_type_id, caller_templates, func_templates, specificity_cost);
				}
			}

			if (ID_IS(caller_type, ID_PLACE_TYPE)) {
				Place_T caller_place = LOOKUP(caller_type, Place_T);
				caller_type = caller_place.basetype_id;
			}

			ID group_id = qualify_symbol(func_sym, ID_SYMBOL_TYPE);
			if (ID_IS(group_id, ID_AST_GROUP)) {
				a_group group = LOOKUP(group_id, a_group);
				ASSERT1(ID_IS(group.type_id, ID_TUPLE_TYPE));
				Tuple_T tuple = LOOKUP(group.type_id, Tuple_T);

				for (size_t i = 0; i < tuple.types.size; ++i) {
					ID tuple_el_id = ARENA_GET(tuple.types, i, ID);
					if (is_valid_equal_type(caller_type, tuple_el_id, caller_templates, func_templates, specificity_cost)) {
						return 1;
					}
				}
			}

			if (!ID_IS(caller_type, ID_SYMBOL_TYPE)) {
				return 0;
			}

			Symbol_T caller = LOOKUP(caller_type, Symbol_T);
			a_symbol * caller_sym = lookup(caller.symbol_id);

			caller_sym->node_id = qualify_symbol(caller_sym, ID_SYMBOL_TYPE);

			// println("caller sym: {s}", ast_to_string(caller.symbol_id));

			if (ID_IS_INVALID(caller_sym->node_id)) {
				FATAL("Unable to find symbol: {s}", ast_to_string(caller.symbol_id));
			}

			func_sym->node_id = qualify_symbol(func_sym, ID_SYMBOL_TYPE);

			if (ID_IS_INVALID(func_sym->node_id)) {
				FATAL("Unable to find symbol: {s}", ast_to_string(func.symbol_id));
			}

			if (!id_is_equal(caller_sym->node_id, func_sym->node_id) || caller.templates.size != func.templates.size) {
				return 0;
			}

			for (size_t i = 0; i < caller.templates.size; ++i) {
				ID caller_template_id = ARENA_GET(caller.templates, i, ID);
				ID func_template_id = ARENA_GET(func.templates, i, ID);

				if (!is_valid_equal_type(caller_template_id, func_template_id, caller_templates, func_templates, specificity_cost)) {
					return 0;
				}
			}

			return 1;
		} break;
		case ID_REF_TYPE: {
			if (!ID_IS(caller_type, ID_REF_TYPE)) {
				return 0;
			}

			Ref_T * caller = lookup(caller_type), func = LOOKUP(func_type, Ref_T);
			if (caller->depth < func.depth) {
				return 0;
			}

			if (caller->depth == func.depth) {
				return caller->is_mut == func.is_mut && is_valid_equal_type(caller->basetype_id, func.basetype_id, caller_templates, func_templates, specificity_cost);
			}

			FATAL("Unsure about this");

			// temporarily change depth (Is safe because ID should not be accessed from somewhere else at the same time since it is within the caller function scope)
			caller->depth -= func.depth;
			char is_equal = is_valid_equal_type(caller_type, func.basetype_id, caller_templates, func_templates, specificity_cost);
			caller->depth += func.depth;

			return is_equal;
		} break;
		case ID_ARRAY_TYPE: {
			if (!ID_IS(caller_type, ID_ARRAY_TYPE)) {
				return 0;
			}
		} break;
		case ID_TUPLE_TYPE: {
			if (!ID_IS(caller_type, ID_TUPLE_TYPE)) {
				return 0;
			}
		} break;
		case ID_PLACE_TYPE: {
			if (!ID_IS(caller_type, ID_PLACE_TYPE)) {
				return 0;
			}

			Place_T caller_place = LOOKUP(caller_type, Place_T), func_place = LOOKUP(func_type, Place_T);
			return caller_place.is_mut == func_place.is_mut && is_valid_equal_type(caller_place.basetype_id, func_place.basetype_id, caller_templates, func_templates, specificity_cost);
		}
		default:
			FATAL("Invalidly trying to typecheck type: '{s}'", id_type_to_string(func_type.type));
			return 0;
	}

	return 1;
}

char is_valid_equal_type(ID caller_type, ID func_type, khash_t(map_id_to_id) * caller_templates, khash_t(map_id_to_id) * func_templates, unsigned int * specificity_cost) {
	if (is_equal_types_and_template_resolution(caller_type, func_type, caller_templates, func_templates, specificity_cost)) {
		return 1;
	}

	// println("Checking template resolution");

	if (ID_IS(caller_type, ID_SYMBOL_TYPE) && caller_templates != NULL) {
		a_symbol * caller_sym = lookup(LOOKUP(caller_type, Symbol_T).symbol_id);
		khint_t found = 0;
		ID template_type_id = get_template_from_templates(*caller_sym, caller_templates, &found);

		if (found != kh_end(caller_templates)) {
			if (ID_IS_INVALID(template_type_id)) {
				if (ID_IS(func_type, ID_PLACE_TYPE)) {
					func_type = LOOKUP(func_type, Place_T).basetype_id;
					ASSERT1(!ID_IS_INVALID(func_type));
				}

				kh_value(caller_templates, found) = template_type_id = func_type;
				// println("[CALLER] {s} <- {s}", type_to_str(caller_type), type_to_str(func_type));
				return 1;
			}

			if (ID_IS(template_type_id, ID_AST_VARIABLE)) {
				a_variable var = LOOKUP(template_type_id, a_variable);
				template_type_id = var.type_id;
			}

			// println("[CALLER] {s} -> {s}", type_to_str(template_type_id), type_to_str(caller_type)); 
			return is_valid_equal_type(template_type_id, func_type, caller_templates, caller_templates, specificity_cost);
		}
	}

	// println("Is #ImplicitCast<{s}, {s}>", type_to_str(caller_type), type_to_str(func_type));

	khash_t(map_id_to_id) temp_templates = kh_init(map_id_to_id);

	ID implicit_cast_trait = context_get_implicit_cast_trait();
	a_trait trait = LOOKUP(implicit_cast_trait, a_trait);
	ASSERT1(trait.templates.size == 2);

	// puts("Checking implicit conversion");
	for (size_t i = 0; i < trait.implementations.size; ++i) {
		kh_clear(map_id_to_id, &temp_templates);
		a_implementation impl = LOOKUP(ARENA_GET(trait.implementations, i, ID), a_implementation);
		// populate_template_hashmap_with_impl(impl, &temp_templates);
		populate_template_hashmap_by_arena(impl.generic_templates, &temp_templates, 0);

		ID id1 = ARENA_GET(impl.trait_templates, 0, ID);
		if (!is_equal_types_and_template_resolution(caller_type, id1, caller_templates, &temp_templates, specificity_cost)) {
			// println("Nope id1 did not match #ImplicitCast<{s}, {s}>", type_to_str(caller_type), type_to_str(func_type));
			continue;
		}

		// println("Found from: {s} | {s}", type_to_str(caller_type), type_to_str(id1));

		ID id2 = ARENA_GET(impl.trait_templates, 1, ID);
		// println("0 impl: #ImplicitCast<{s}, {s}>", type_to_str(id1), type_to_str(id2));
		if (is_equal_types_and_template_resolution(func_type, id2, func_templates, &temp_templates, specificity_cost)) {
			// println("Found to: {s} | {s}", type_to_str(func_type), type_to_str(id2));
			*specificity_cost += 1; // Add to specificity because we needed to implicitly cast
			return 1;
		}
	}

	kh_free(map_id_to_id, &temp_templates);

	return 0;
}

char type_has_trait_implementation(ID type_id, ID trait_id) {
	if (ID_IS(trait_id, ID_AST_IMPL)) {
		FATAL("This has been removed");
		// a_implementation impl = LOOKUP(trait_id, a_implementation);
		// ASSERT1(!ID_IS_INVALID(impl.trait_symbol_id));
		// a_symbol symbol = LOOKUP(impl.trait_symbol_id, a_symbol);
		// ASSERT1(ID_IS(impl.type_id, ID_TUPLE_TYPE));
		//
		// Tuple_T tuple = LOOKUP(impl.type_id, Tuple_T);
		// for (size_t i = 0; i < tuple.types.size; ++i) {
		// 	ID impl_type = ARENA_GET(tuple.types, i, ID);
		// 	if (!is_equal_types_and_template_resolution(type_id, impl_type, template_rules, NULL)) {
		// 		return 0;
		// 	}
		// }
		//
		// return 1;
	}

	ASSERT1(ID_IS(trait_id, ID_AST_TRAIT));

	a_trait trait = LOOKUP(trait_id, a_trait);
	
	for (size_t i = 0; i < trait.implementations.size; ++i) {
		ID impl_id = ARENA_GET(trait.implementations, i, ID);
		if (type_has_trait_implementation(type_id, impl_id)) {
			return 1;
		}
	}

	return 0;
}

ID resolve_type_templates_in_type(ID type_id, khash_t(map_id_to_id) * templates) {
	// println("resolve type templates in: {s}", id_type_to_string(type_id.type));

	switch (type_id.type) {
		case ID_NUMERIC_TYPE:
			return type_id;
		case ID_TUPLE_TYPE:
			FATAL("Not implemented yet");
		case ID_ARRAY_TYPE: {
			Array_T array = LOOKUP(type_id, Array_T);
			ID resolved_type = resolve_type_templates_in_type(array.basetype_id, templates);
			if (id_is_equal(resolved_type, array.basetype_id)) {
				return type_id; // Nothing changed
			}

			Array_T * new_array = type_allocate(ID_ARRAY_TYPE);
			new_array->basetype_id = resolved_type;
			new_array->size = array.size;

			return new_array->info.type_id;
		}
		case ID_REF_TYPE: {
			Ref_T ref = LOOKUP(type_id, Ref_T);
			ID resolved_type = resolve_type_templates_in_type(ref.basetype_id, templates);
			if (id_is_equal(resolved_type, ref.basetype_id)) {
				return type_id; // Nothing changed
			}

			Ref_T * new_ref = type_allocate(ID_REF_TYPE);
			new_ref->basetype_id = resolved_type;
			new_ref->depth = ref.depth;
			new_ref->is_mut = ref.is_mut;

			return new_ref->info.type_id;
		}
		case ID_PLACE_TYPE: {
			Place_T place = LOOKUP(type_id, Place_T);
			ID resolved_type = resolve_type_templates_in_type(place.basetype_id, templates);
			if (id_is_equal(place.basetype_id, resolved_type)) {
				return type_id; // Nothing changed
			}

			Place_T * new_place = type_allocate(ID_PLACE_TYPE);
			new_place->basetype_id = resolved_type;
			new_place->is_mut = place.is_mut;

			return new_place->info.type_id;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol_type = LOOKUP(type_id, Symbol_T);
			if (symbol_type.templates.size != 0) {
				Symbol_T * new_symbol = type_allocate(ID_SYMBOL_TYPE);
				new_symbol->symbol_id = symbol_type.symbol_id;
				new_symbol->templates = arena_init(sizeof(ID));
				arena_grow(&new_symbol->templates, symbol_type.templates.size);

				for (size_t i = 0; i < symbol_type.templates.size; ++i) {
					ARENA_APPEND(&new_symbol->templates, resolve_type_templates_in_type(ARENA_GET(symbol_type.templates, i, ID), templates));
				}

				return new_symbol->info.type_id;
			}

			a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);

			khint_t found;
			ID template_type = get_template_from_templates(symbol, templates, &found);
			if (found == kh_end(templates)) { // Is not a template
				return type_id;
			}

			if (ID_IS(template_type, ID_AST_VARIABLE)) {
				template_type = LOOKUP(template_type, a_variable).type_id;
			}

			ASSERT1(!ID_IS_INVALID(template_type));
			return resolve_type_templates_in_type(template_type, templates);
		}
		case ID_VOID_TYPE:
			return type_id;
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(type_id.type));
	}
}


ID get_template_from_templates(a_symbol symbol, khash_t(map_id_to_id) * templates, khint_t * found) {
	ASSERT1(!ID_IS_INVALID(symbol.name_id));

	khint_t key = kh_get(map_id_to_id, templates, symbol.name_id);
	if (found != NULL) {
		*found = key;
	}

	if (key == kh_end(templates)) {
		return INVALID_ID;
	}

	return kh_value(templates, key);
}

char check_templates_uphold_template_rules(khash_t(map_id_to_id) templates, Arena template_rules) {
	for (size_t i = 0; i < template_rules.size; ++i) {
		ID rule_id = ARENA_GET(template_rules, i, ID);
		ASSERT1(ID_IS(rule_id, ID_AST_SYMBOL));
		a_symbol symbol = LOOKUP(rule_id, a_symbol);

		if (ID_IS_INVALID(symbol.node_id)) {
			continue; // Automatic pass if there was no rule for this template
		}

		a_variable var = LOOKUP(symbol.node_id, a_variable);
		ID type_id = get_template_from_templates(symbol, &templates, NULL);

		ASSERT1(ID_IS(var.type_id, ID_SYMBOL_TYPE));
		a_symbol * type_rule_symbol = lookup(LOOKUP(var.type_id, Symbol_T).symbol_id);

		ID type_rule_id = qualify_symbol(type_rule_symbol, ID_SYMBOL_TYPE);

		ASSERT1(!ID_IS_INVALID(type_id));
		ASSERT1(!ID_IS_INVALID(type_rule_id));

		unsigned int specificity_score = 0;
		switch (type_rule_id.type) {
			case ID_AST_GROUP:
				// Can never implicitly convert into group so keep this as this
				if (!is_equal_types_and_template_resolution(type_id, var.type_id, NULL, &templates, &specificity_score)) {
					ASSERT1(specificity_score == 0);
					return 0;
				}
				break;
			default:
				ERROR("Invalid rule type for template type \"{s}\"", interner_lookup_str(symbol.name_id)._ptr);
				print_trace();
				exit(1);
		}
	}

	return 1;
}

void populate_template_hashmap_by_arena(Arena arena, khash_t(map_id_to_id) * templates, char set_type) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID symbol_id = ARENA_GET(arena, i, ID);
		a_symbol template = LOOKUP(symbol_id, a_symbol);

		// println("{i}) {s}: {s}", i, interner_lookup_str(template.name_id)._ptr, ID_IS_INVALID(template.node_id) ? "?" : ast_to_string(template.node_id));

		int retcode;
		khint_t k = kh_put(map_id_to_id, templates, template.name_id, &retcode);

		if (retcode == KH_PUT_ALREADY_PRESENT) {
			ERROR("Duplicate template type: '{s}'", interner_lookup_str(template.name_id)._ptr);
			exit(1);
		}

		ASSERT(retcode == KH_PUT_SUCCESS, "Unknown error occured while populating template hashmap: {i}", retcode);
		kh_value(templates, k) = (set_type ? template.node_id : INVALID_ID);
	}
}

void populate_template_hashmap_with_impl(a_implementation impl, khash_t(map_id_to_id) * templates) {
	// Since trait's are already cached this should be okay
	ID trait_id = qualify_symbol(lookup(impl.trait_symbol_id), ID_SYMBOL_TYPE);
	a_trait trait = LOOKUP(trait_id, a_trait);

	for (size_t i = 0; i < impl.trait_templates.size; ++i) {
		ID type_id = ARENA_GET(impl.trait_templates, i, ID);
		ID name_id = LOOKUP(ARENA_GET(trait.templates, i, ID), a_symbol).name_id;

		// println("{i}) {s}: {s}", i, interner_lookup_str(name_id)._ptr, type_to_str(type_id));

		int retcode;
		khint_t k = kh_put(map_id_to_id, templates, name_id, &retcode);

		if (retcode == KH_PUT_ALREADY_PRESENT) {
			ERROR("Duplicate template type: '{s}'", interner_lookup_str(name_id)._ptr);
			exit(1);
		}

		ASSERT(retcode == KH_PUT_SUCCESS, "Unknown error occured while populating template hashmap: {i}", retcode);
		kh_value(templates, k) = type_id;
	}

	populate_template_hashmap_by_arena(impl.generic_templates, templates, 0);
}
