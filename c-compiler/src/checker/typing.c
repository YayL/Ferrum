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

char is_equal_types_and_template_resolution(ID caller_type, ID func_type, khash_t(map_id_to_id) * template_rules, khash_t(map_id_to_id) * templates);
char type_has_trait_implementation(ID type, ID trait, khash_t(map_id_to_id) * template_rules);
char check_template_def_matches_type(ID template_id, ID type_id);
ID resolve_type_templates_in_type(ID type_id, khash_t(map_id_to_id) * templates);
ID get_template_from_templates(a_symbol symbol, khash_t(map_id_to_id) * templates, khint_t * found);
void populate_template_rules_hashmap(Arena arena, khash_t(map_id_to_id) * template_rules);

char frsolver_check_candidate(FRSolver solver, a_function candidate, khash_t(map_id_to_id) * template_rules, khash_t(map_id_to_id) * templates) {
	ASSERT1(ID_IS(solver.args_type_id, ID_TUPLE_TYPE));
	ASSERT1(ID_IS(candidate.param_type, ID_TUPLE_TYPE));

	Arena call_arg_types = LOOKUP(solver.args_type_id, Tuple_T).types;
	Arena func_arg_types = LOOKUP(candidate.param_type, Tuple_T).types;

	// Must have same amount of arguments
	if (call_arg_types.size != func_arg_types.size) {
		return 0;
	}

	/* Perform function argument type checking */
	for (size_t i = 0; i < call_arg_types.size; ++i) {
		ID call_arg_type = ARENA_GET(call_arg_types, i, ID);
		ASSERT1(!ID_IS_INVALID(call_arg_type));
		ID func_arg_type = ARENA_GET(func_arg_types, i, ID);
		ASSERT1(!ID_IS_INVALID(func_arg_type));

		if (!is_equal_types_and_template_resolution(call_arg_type, func_arg_type, template_rules, templates)) {
			return 0;
		}
	}

	return 1;
}

FRResult frsolver_solve(FRSolver solver) {
	FRResult result = frresult_init(solver);
	khash_t(map_id_to_id) templates = kh_init(map_id_to_id);
	khash_t(map_id_to_id) template_rules = kh_init(map_id_to_id);

	for (size_t i = 0; i < solver.candidates.size; ++i) {
		ID candidate_id = ARENA_GET(solver.candidates, i, ID);
		ASSERT1(ID_IS(candidate_id, ID_AST_FUNCTION));

		a_function candidate = LOOKUP(candidate_id, a_function);
		kh_clear(map_id_to_id, &template_rules);

		populate_template_rules_hashmap(candidate.templates, &template_rules);

		switch (candidate.info.scope_id.type) {
		case ID_AST_IMPL:
			populate_template_rules_hashmap(LOOKUP(candidate.info.scope_id, a_implementation).impl_templates, &template_rules); break;
		default: break;
		}
		
		println("Candidate: {s}, templates: {u}", interner_lookup_str(candidate.name_id)._ptr, template_rules.size);

		if (frsolver_check_candidate(solver, candidate, &template_rules, &templates)) {
			println("Found: {s}", ast_to_string(candidate_id));

			ID key, value;
			kh_foreach(&templates, key, value, {
				ASSERT1(!ID_IS_INVALID(key));
				if (ID_IS_INVALID(value)) {
					println("Unable to determine type '{s}'", interner_lookup_str(key)._ptr);
					continue;
				}

				println("{s}: {s}", interner_lookup_str(key)._ptr, type_to_str(value));
			});

			if (!ID_IS_INVALID(result.function_id)) {
				ERROR("There are multiple function candidates, unable to resolve");
				exit(1);
			}

			result.function_return_type_id = resolve_type_templates_in_type(candidate.return_type, &templates);
			result.function_id = candidate_id;
			/* TODO: Templates have been resolved so keep that information (Required for codegen) */
		}
	}

	kh_free(map_id_to_id, &templates);

	return result;
}

char is_equal_types_and_template_resolution(ID caller_type, ID func_type, khash_t(map_id_to_id) * template_rules, khash_t(map_id_to_id) * templates) {
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
			a_symbol * func_sym = lookup(func.symbol_id);

			if (templates != NULL) {
				khint_t found = 0; // the kh_value key of the templates lookup
				ID template_type_id = get_template_from_templates(*func_sym, templates, &found);

				if (found != kh_end(templates)) { // If is a template (this is not equivalent to ID_IS_INVALID(template_type_id) because templates is populated with empty values at the start)
					if (ID_IS_INVALID(template_type_id)) {
						kh_value(templates, found) = template_type_id = caller_type;
						println("{s} -> {s}", type_to_str(func_type), type_to_str(caller_type));
					} else {
						println("{s}[{s}] -> {s}", type_to_str(func_type), type_to_str(template_type_id), type_to_str(caller_type));
					}

					return is_equal_types_and_template_resolution(caller_type, template_type_id, template_rules, templates);
				}
			}

			if (!ID_IS(caller_type, ID_SYMBOL_TYPE)) {
				return 0;
			}

			Symbol_T caller = LOOKUP(caller_type, Symbol_T);
			a_symbol * caller_sym = lookup(caller.symbol_id);

			caller_sym->node_id = qualify_symbol(*caller_sym, ID_SYMBOL_TYPE);

			if (ID_IS_INVALID(caller_sym->node_id)) {
				FATAL("Unable to find symbol: {s}", ast_to_string(caller.symbol_id));
			}

			func_sym->node_id = qualify_symbol(*func_sym, ID_SYMBOL_TYPE);

			if (ID_IS_INVALID(func_sym->node_id)) {
				FATAL("Unable to find symbol: {s}", ast_to_string(func.symbol_id));
			}

			if (!id_is_equal(caller_sym->node_id, func_sym->node_id) || caller.templates.size != func.templates.size) {
				return 0;
			}

			for (size_t i = 0; i < caller.templates.size; ++i) {
				ID caller_template_id = ARENA_GET(caller.templates, i, ID);
				ID func_template_id = ARENA_GET(func.templates, i, ID);

				if (!is_equal_types_and_template_resolution(caller_template_id, func_template_id, template_rules, templates)) {
					return 0;
				}
			}

			print_ast_tree(caller.symbol_id);
			print_ast_tree(func.symbol_id);

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
				return is_equal_types_and_template_resolution(caller->basetype_id, func.basetype_id, template_rules, templates);
			}

			FATAL("Unsure about this");

			// temporarily change depth (Is safe because ID should not be accessed from somewhere else at the same time since it is within the caller function scope)
			caller->depth -= func.depth;
			char is_equal = is_equal_types_and_template_resolution(caller_type, func.basetype_id, template_rules, templates);
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

			return type_has_trait_implementation(caller_type, trait_id, template_rules);

		} break;
		default:
			FATAL("Invalidly trying to typecheck type: '{s}'", id_type_to_string(func_type.type));
			return 0;
	}

	return 1;
}

char type_has_trait_implementation(ID type_id, ID trait_id, khash_t(map_id_to_id) * template_rules) {
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
		if (type_has_trait_implementation(type_id, impl_id, template_rules)) {
			return 1;
		}
	}

	return 0;
}

char check_template_def_matches_type(ID template_id, ID type_id) {
	println("template: {s}", ast_to_string(template_id));

	return 1;
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

			return resolve_type_templates_in_type(type_id, templates);
		}
		case ID_IMPL_TYPE:
			FATAL("Not implemented");
		case ID_VOID_TYPE:
			return type_id;
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(type_id.type));
	}
}


ID get_template_from_templates(a_symbol symbol, khash_t(map_id_to_id) * templates, khint_t * found) {
	ASSERT1(!ID_IS_INVALID(symbol.name_id));
	*found = kh_get(map_id_to_id, templates, symbol.name_id);

	if (*found == kh_end(templates)) {
		return INVALID_ID;
	}

	return kh_value(templates, *found);
}

void populate_template_rules_hashmap(Arena arena, khash_t(map_id_to_id) * template_rules) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID symbol_id = ARENA_GET(arena, i, ID);
		a_symbol template = LOOKUP(symbol_id, a_symbol);

		// println("Adding: {s} | {s} | {s}", interner_lookup_str(template.name_id)._ptr, ast_to_string(symbol_id), type_to_str(template.node_id));

		int retcode;
		khint_t k = kh_put(map_id_to_id, template_rules, template.name_id, &retcode);

		if (retcode == KH_PUT_ALREADY_PRESENT) {
			ERROR("Duplicate template type: '{s}'", interner_lookup_str(template.name_id)._ptr);
			exit(1);
		}

		ASSERT(retcode == KH_PUT_SUCCESS, "Unknown error occured while populating template hashmap: {i}", retcode);
		kh_value(template_rules, k) = template.node_id;
	}
}
