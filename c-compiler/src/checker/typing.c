#include "checker/typing.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"
#include "checker/symbol.h"

FRSolver frsolver_init(ID name_id, ID args_type_id, ID scope_id, Arena candidates) {
	a_module * module = get_scope(ID_AST_MODULE, scope_id);
	ASSERT1(module != NULL);

	return (FRSolver) { .name_id = name_id, args_type_id = args_type_id, .candidates = candidates };
}

FRResult frresult_init(FRSolver solver) {
	return (FRResult) { .name_id = solver.name_id, .args_type_id = solver.args_type_id, .function_id = INVALID_ID };
}

char is_equal_types_and_template_resolution(ID caller_type, ID func_type, khash_t(map_id_to_id) * templates);
char type_has_trait_implementation(ID type, ID trait);
ID resolve_type_templates_in_type(ID type_id, khash_t(map_id_to_id) * templates);

char frsolver_check_candidate(FRSolver solver, a_function candidate, khash_t(map_id_to_id) * templates) {
	ASSERT1(ID_IS(solver.args_type_id, ID_TUPLE_TYPE));
	ASSERT1(ID_IS(candidate.param_type, ID_TUPLE_TYPE));

	Arena call_arg_types = LOOKUP(solver.args_type_id, Tuple_T).types;
	Arena func_arg_types = LOOKUP(candidate.param_type, Tuple_T).types;

	// Must have same amount of arguments
	if (call_arg_types.size != func_arg_types.size) {
		return 0;
	}

	for (size_t i = 0; i < call_arg_types.size; ++i) {
		ID call_arg_type = ARENA_GET(call_arg_types, i, ID);
		ASSERT1(!ID_IS_INVALID(call_arg_type));
		ID func_arg_type = ARENA_GET(func_arg_types, i, ID);
		ASSERT1(!ID_IS_INVALID(func_arg_type));

		if (!is_equal_types_and_template_resolution(call_arg_type, func_arg_type, templates)) {
			return 0;
		}
	}

	/* Perform trait bound checking here */
	for (size_t i = 0; i < candidate.templates.size; ++i) {
		ID symbol_id = ARENA_GET(candidate.templates, i, ID);
		a_symbol template = LOOKUP(symbol_id, a_symbol);

		if (ID_IS_INVALID(template.node_id)) {
			continue;
		}

		khint_t k = kh_get(map_id_to_id, templates, template.name_id);
		ASSERT1(k != kh_end(templates));
		ID type = kh_value(templates, k);

		if (!type_has_trait_implementation(type, template.node_id)) {
			return 0;
		}
	}

	return 1;
}

FRResult frsolver_solve(FRSolver solver) {
	FRResult result = frresult_init(solver);
	khash_t(map_id_to_id) templates = kh_init(map_id_to_id);

	for (size_t i = 0; i < solver.candidates.size; ++i) {
		ID candidate_id = ARENA_GET(solver.candidates, i, ID);
		ASSERT1(ID_IS(candidate_id, ID_AST_FUNCTION));

		a_function candidate = LOOKUP(candidate_id, a_function);
		kh_clear(map_id_to_id, &templates);

		// Add templates in arena to templates hashmap
		for (size_t i = 0; i < candidate.templates.size; ++i) {
			ID symbol_id = ARENA_GET(candidate.templates, i, ID);
			a_symbol template = LOOKUP(symbol_id, a_symbol);

			int retcode;
			khint_t k = kh_put(map_id_to_id, &templates, template.name_id, &retcode);
			ASSERT1(retcode == KH_PUT_SUCCESS); // Should be cleared?
			kh_value(&templates, k) = INVALID_ID;
		}

		if (frsolver_check_candidate(solver, candidate, &templates)) {
			if (!ID_IS_INVALID(result.function_id)) {
				ERROR("There are multiple function candidates, unable to resolve");
				exit(1);
			}

			result.function_return_type_id = resolve_type_templates_in_type(candidate.return_type, &templates);
			result.function_id = candidate_id;
		}
	}

	kh_free(map_id_to_id, &templates);

	return result;
}

ID get_template_from_templates(a_symbol symbol, khash_t(map_id_to_id) * templates, khint_t * found) {
	ASSERT1(!ID_IS_INVALID(symbol.name_id));
	*found = kh_get(map_id_to_id, templates, symbol.name_id);

	if (*found == kh_end(templates)) {
		return INVALID_ID;
	}

	return kh_value(templates, *found);
}

char is_equal_types_and_template_resolution(ID caller_type, ID func_type, khash_t(map_id_to_id) * templates) {
	switch (func_type.type) {
		case ID_NUMERIC_TYPE: {
			if (!ID_IS(caller_type, ID_NUMERIC_TYPE)) {
				return 0;
			}
			
			Numeric_T caller = LOOKUP(caller_type, Numeric_T), func = LOOKUP(func_type, Numeric_T);
			return caller.type == func.type && caller.type == func.type;
		} break;
		case ID_SYMBOL_TYPE: {
			Symbol_T func = LOOKUP(func_type, Symbol_T);
			a_symbol func_sym = LOOKUP(func.symbol_id, a_symbol);

			if (templates != NULL) {
				khint_t found = 0; // the kh_value key of the templates lookup
				ID template_type_id = get_template_from_templates(func_sym, templates, &found);
				if (found != kh_end(templates)) { // If is a template
					if (ID_IS_INVALID(template_type_id)) {
						kh_value(templates, found) = template_type_id = caller_type;
					}

					return is_equal_types_and_template_resolution(caller_type, template_type_id, templates);
				} else if (!ID_IS(caller_type, ID_SYMBOL_TYPE)) { // Caller type is not a symbol
					return 0;
				}
			}

			FATAL("Not implemented");
			
			Symbol_T caller = LOOKUP(caller_type, Symbol_T);
			a_symbol caller_sym = LOOKUP(caller.symbol_id, a_symbol);

			ASSERT1(!ID_IS_INVALID(func_sym.node_id));
			ASSERT1(!ID_IS_INVALID(caller_sym.node_id));
			return id_is_equal(func_sym.node_id, caller_sym.node_id);
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
				return is_equal_types_and_template_resolution(caller->basetype_id, func.basetype_id, templates);
			}

			FATAL("Unsure about this");

			// temporarily change depth because this type ID should not be accessed from somewhere else at the same time
			caller->depth -= func.depth;
			char is_equal = is_equal_types_and_template_resolution(caller_type, func.basetype_id, templates);
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
		case ID_IMPL_TYPE: {
			if (ID_IS(caller_type, ID_IMPL_TYPE)) {
				FATAL("Trait subset checker has not be implemented yet");
				return 1;
			}

			Impl_T impl = LOOKUP(func_type, Impl_T);

			ASSERT1(ID_IS(impl.symbol_id, ID_AST_SYMBOL));
			a_symbol symbol = LOOKUP(impl.symbol_id, a_symbol);

			ID trait_id = qualify_symbol(symbol, ID_AST_TRAIT);

			if (ID_IS_INVALID(trait_id)) {
				FATAL("Unable to find trait: '{s}'", interner_lookup_str(symbol.name_id)._ptr);
			}

			return type_has_trait_implementation(caller_type, trait_id);

		} break;
		default:
			FATAL("Invalidly trying to typecheck type: '{s}'", id_type_to_string(func_type.type));
			return 0;
	}

	return 1;
}

char type_has_trait_implementation(ID type_id, ID trait_id) {
	if (ID_IS(trait_id, ID_AST_IMPL)) {
		a_implementation impl = LOOKUP(trait_id, a_implementation);
		ASSERT1(!ID_IS_INVALID(impl.trait_symbol_id));
		a_symbol symbol = LOOKUP(impl.trait_symbol_id, a_symbol);
		ASSERT1(ID_IS(impl.type_id, ID_TUPLE_TYPE));

		Tuple_T tuple = LOOKUP(impl.type_id, Tuple_T);
		for (size_t i = 0; i < tuple.types.size; ++i) {
			ID impl_type = ARENA_GET(tuple.types, i, ID);
			if (!is_equal_types_and_template_resolution(type_id, impl_type, NULL)) {
				return 0;
			}
		}

		return 1;
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
	switch (type_id.type) {
		case ID_NUMERIC_TYPE:
			return type_id;
		case ID_TUPLE_TYPE:
			FATAL("Not implemented yet");
		case ID_ARRAY_TYPE: {
			Array_T array = LOOKUP(type_id, Array_T);
			ID resolved_type = resolve_type_templates_in_type(array.basetype_id, templates);
			if (id_is_equal(resolved_type, array.basetype_id)) {
				return type_id;
			}

			Array_T * new_array = type_allocate(ID_ARRAY_TYPE, array.info.is_mut);
			new_array->basetype_id = resolved_type;
			new_array->size = array.size;

			return new_array->info.type_id;
		}
		case ID_REF_TYPE: {
			Ref_T ref = LOOKUP(type_id, Ref_T);
			ID resolved_type = resolve_type_templates_in_type(ref.basetype_id, templates);
			if (id_is_equal(resolved_type, ref.basetype_id)) {
				return type_id;
			}

			Ref_T * new_ref = type_allocate(ID_ARRAY_TYPE, ref.info.is_mut);
			new_ref->basetype_id = resolved_type;
			new_ref->depth = ref.depth;

			return new_ref->info.type_id;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol_type = LOOKUP(type_id, Symbol_T);
			a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);

			khint_t found;
			ID template_type = get_template_from_templates(symbol, templates, &found);
			if (found != kh_end(templates)) { // Is a template
				ASSERT1(!ID_IS_INVALID(template_type));
				return template_type;
			}

			return type_id;
		}
		case ID_IMPL_TYPE:
			FATAL("Not implemented");
		case ID_VOID_TYPE:
			return type_id;
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(type_id.type));
	}
}
