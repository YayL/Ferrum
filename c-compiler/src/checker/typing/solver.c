#include "checker/typing/solver.h"

#include "common/ID.h"
#include "tables/registry_manager.h"
#include "checker/typing/subtyping.h"
#include "checker/symbol.h"

Solver solver_initialize() {
	return (Solver) {
		.context_element_count = 0
	};
}

ID solver_find_symbol_term_var(Solver * solver, ID symbol_id) {
	ID interner_id = ast_get_interner_id(symbol_id);
	LOOP_OVER_REGISTRY_REV(TermVar, term_var, {
		if (ID_IS_EQUAL(ast_get_interner_id(term_var->symbol_id), interner_id)) {
			return term_var->info.id;
		}
	});

	return INVALID_ID;
}

#define MARKER_SET_ITEM_COUNT(ENUM, _, TYPE, ...) REGISTRY_MANAGER_SET_ITEM_COUNT(TYPE, marker.MARKER_COUNT_GET(TYPE));
void solver_reset_to_marker(Solver * solver, ID marker_id) {
	ASSERT1(ID_IS(marker_id, ID_MARKER));
	Marker marker = LOOKUP(marker_id, Marker);

	TYPE_CHECKING_CONTEXT_KINDS(MARKER_SET_ITEM_COUNT);
}

static inline void _collect_templates(Solver * solver, const Arena node_templates) {
	for (size_t i = 0; i < node_templates.size; ++i) {
		ID template_symbol_id = ARENA_GET(node_templates, i, ID);
		ASSERT1(ID_IS(template_symbol_id, ID_AST_SYMBOL));
		a_symbol symbol = LOOKUP(template_symbol_id, a_symbol);
		ASSERT1(symbol.name_ids.size == 1);

		// if (!ID_IS_INVALID(solver_get_template_type(symbol.name_id))) {
		// 	ERROR("Ambigious template \"{s}\"; duplicate template.", interner_lookup_str(symbol.name_id)._ptr);
		// 	exit(1);
		// }

		Template * template = solver_allocate(solver, ID_TEMPLATE);
		template->name_id = symbol.name_id;

		if (!ID_IS_INVALID(symbol.node_id)) {
			ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
			a_variable variable = LOOKUP(symbol.node_id, a_variable);

			ASSERT1(!ID_IS_INVALID(variable.type_id));
			template->type_id = solver_replace_templates(variable.type_id);
		} else {
			template->type_id = ((Existential *) solver_allocate(solver, ID_EXISTENIAL))->info.id;
		}
	}
}

void solver_collect_templates(Solver * solver, ID node_id) {
	if (ID_IS(node_id, ID_AST_MODULE)) {
		return;
	}

	solver_collect_templates(solver, ast_get_scope_id(node_id));

	switch (node_id.type) {
		case ID_AST_FUNCTION: _collect_templates(solver, LOOKUP(node_id, a_function).templates); break;
		case ID_AST_STRUCT: _collect_templates(solver, LOOKUP(node_id, a_structure).templates); break;
		case ID_AST_IMPL: {
			a_implementation impl = LOOKUP(node_id, a_implementation);
			_collect_templates(solver, impl.generics);
			_collect_templates(solver, impl.templates);
		} break;
		default: FATAL("Unimplemented node: {s}", id_type_to_string(node_id.type));
	}
}

ID solver_get_template_type(ID name_id) {
	LOOP_OVER_REGISTRY_REV(Template, template, {
		if (ID_IS_EQUAL(template->name_id, name_id)) {
			return template->type_id;
		}
	});

	return INVALID_ID;
}

char solver_implementation_is_valid(Solver * solver, ID implementation_id, const Arena templates) {
	ASSERT1(ID_IS(implementation_id, ID_AST_IMPL));
	const a_implementation impl = LOOKUP(implementation_id, a_implementation);

	if (impl.templates.size != templates.size) {
		return 0;
	}

	Marker * marker = solver_allocate(solver, ID_MARKER);

	ASSERT1(ID_IS(impl.info.scope_id, ID_AST_MODULE));
	_collect_templates(solver, impl.generics);

	for (size_t i = 0; i < templates.size; ++i) {
		ID template_symbol_id = ARENA_GET(impl.templates, i, ID);
		ASSERT1(ID_IS(template_symbol_id, ID_AST_SYMBOL));
		a_symbol symbol = LOOKUP(template_symbol_id, a_symbol);
		ASSERT1(ID_IS(symbol.node_id, ID_AST_VARIABLE));
		a_variable variable = LOOKUP(symbol.node_id, a_variable);
		ASSERT1(!ID_IS_INVALID(variable.type_id));

		ID fixed_template_type_id = solver_replace_templates(variable.type_id);
		const ID template_id = ARENA_GET(templates, i, ID);
		// println("{u}) {s} <: {s}", i + 1, type_to_str(template_id), type_to_str(fixed_template_type_id));
		if (!is_subtype_strict(solver, template_id, fixed_template_type_id)) {
			// println("\t nope");
			solver_reset_to_marker(solver, marker->info.id);
			return 0;
		}
		// println("\t yup");
	}

	for (size_t i = 0; i < impl.where.size; ++i) {
		if (!solver_validate_trait_implementation(solver, ARENA_GET(impl.where, i, ID))) {
			return 0;
		}
	}

	solver_reset_to_marker(solver, marker->info.id);
	return 1;
}

char solver_validate_trait_implementation(Solver * solver, ID symbol_type_id) {
	ASSERT1(ID_IS(symbol_type_id, ID_SYMBOL_TYPE));
	Symbol_T symbol_type = LOOKUP(symbol_type_id, Symbol_T);
	ASSERT1(!ID_IS_INVALID(symbol_type.symbol_id));
	a_symbol * symbol = lookup(symbol_type.symbol_id);

	if (ID_IS_INVALID(qualify_symbol(symbol, ID_SYMBOL_TYPE))) {
		FATAL("Unable to qualify: {s}", ast_to_string(symbol_type.symbol_id));
	}

	ASSERT(ID_IS(symbol->node_id, ID_AST_TRAIT), "Non trait found in where clause");
	const a_trait trait = LOOKUP(symbol->node_id, a_trait);

	ASSERT1(symbol_type.templates.size > 0);
	Arena templates = arena_init(sizeof(ID));
	arena_grow(&templates, symbol_type.templates.size);

	for (size_t i = 0; i < symbol_type.templates.size; ++i) {
		ARENA_APPEND(&templates, solver_replace_templates(ARENA_GET(symbol_type.templates, i, ID)));
	}

	for (size_t i = 0; i < trait.implementations.size; ++i) {
		if (solver_implementation_is_valid(solver, ARENA_GET(trait.implementations, i, ID), templates)) {
			return 1;
		}
	}

	return 0;
}

static inline char solver_validate_where_clause(Solver * solver, const Arena clauses) {
	for (size_t i = 0; i < clauses.size; ++i) {
		if (!solver_validate_trait_implementation(solver, ARENA_GET(clauses, i, ID))) {
			return 0;
		}
	}

	return 1;
}

char solver_validate_where_clauses(Solver * solver, ID node_id) {
	switch (node_id.type) {
		case ID_AST_FUNCTION: {
			a_function function = LOOKUP(node_id, a_function);
			return solver_validate_where_clause(solver, function.where) 
					&& solver_validate_where_clauses(solver, function.info.scope_id);
		} break;
		case ID_AST_IMPL: {
			a_implementation impl = LOOKUP(node_id, a_implementation);
			return solver_validate_where_clause(solver, impl.where)
					&& solver_validate_where_clauses(solver, impl.info.scope_id);
		}
		case ID_AST_STRUCT: {
			a_structure _struct = LOOKUP(node_id, a_structure);
			return solver_validate_where_clause(solver, _struct.where);
		}
		case ID_AST_MODULE: return 1;
		default: FATAL("Unimplemented scope \"{s}\"", id_type_to_string(node_id.type));
	}
}

#include "checker/symbol.h"

ID solver_replace_templates(ID type_id) {
	switch (type_id.type) {
		case ID_NUMERIC_TYPE:
		case ID_TERM_VAR:
		case ID_EXISTENIAL:
		case ID_VOID_TYPE:
				return type_id;
		case ID_TUPLE_TYPE: {
			Tuple_T tuple = LOOKUP(type_id, Tuple_T);

			ID fixed_type_id = INVALID_ID;
			size_t failed_index = -1;

			for (size_t i = 0; i < tuple.types.size; ++i) {
				ID type_id = ARENA_GET(tuple.types, i, ID);
				fixed_type_id = solver_replace_templates(type_id);
				if (!ID_IS_EQUAL(type_id, fixed_type_id)) {
					failed_index = i;
					break;
				}
			}

			if (failed_index == -1) {
					return type_id;
			}

			Tuple_T * new_tuple = type_allocate(ID_TUPLE_TYPE);
			arena_grow(&new_tuple->types, tuple.types.size);

			// Fill in known equal
			for (size_t i = 0; i < failed_index; ++i) {
				ID type_id = ARENA_GET(tuple.types, i, ID);
				ARENA_APPEND(&new_tuple->types, type_id);
			}

			// Append known fixed
			ASSERT1(!ID_IS_INVALID(fixed_type_id));
			ARENA_APPEND(&new_tuple->types, fixed_type_id);

			// Fill in rest unknown fixed
			for (size_t i = failed_index + 1; i < tuple.types.size; ++i) {
				ID type_id = ARENA_GET(tuple.types, i, ID);
				fixed_type_id = solver_replace_templates(type_id);
				ARENA_APPEND(&new_tuple->types, fixed_type_id);
			}

			return new_tuple->info.type_id;
		}
		case ID_ARRAY_TYPE: {
			Array_T array = LOOKUP(type_id, Array_T);
			ID fixed_type = solver_replace_templates(array.basetype_id);
			if (ID_IS_EQUAL(fixed_type, array.basetype_id)) {
				return type_id; // Nothing changed
			}

			Array_T * new_array = type_allocate(ID_ARRAY_TYPE);
			new_array->basetype_id = fixed_type;
			new_array->size = array.size;

			return new_array->info.type_id;
		}
		case ID_REF_TYPE: {
				Ref_T ref = LOOKUP(type_id, Ref_T);
				ID fixed_type = solver_replace_templates(ref.basetype_id);
				if (ID_IS_EQUAL(fixed_type, ref.basetype_id)) {
						return type_id; // Nothing changed
				}

				Ref_T * new_ref = type_allocate(ID_REF_TYPE);
				new_ref->basetype_id = fixed_type;
				new_ref->depth = ref.depth;
				new_ref->is_mut = ref.is_mut;

				return new_ref->info.type_id;
		}
		case ID_PLACE_TYPE: {
			Place_T place = LOOKUP(type_id, Place_T);
			ID fixed_type = solver_replace_templates(place.basetype_id);
			if (ID_IS_EQUAL(place.basetype_id, fixed_type)) {
				return type_id; // Nothing changed
			}

			Place_T * new_place = type_allocate(ID_PLACE_TYPE);
			new_place->basetype_id = fixed_type;
			new_place->is_mut = place.is_mut;

			return new_place->info.type_id;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T * symbol_type = lookup(type_id);
			a_symbol * symbol = lookup(symbol_type->symbol_id);

			if (ID_IS_INVALID(symbol->node_id)) {
				qualify_symbol(symbol, ID_SYMBOL_TYPE);
			}

			if (symbol_type->templates.size != 0) {
				Symbol_T * new_symbol = type_allocate(ID_SYMBOL_TYPE);
				new_symbol->symbol_id = symbol_type->symbol_id;
				new_symbol->templates = arena_init(sizeof(ID));
				arena_grow(&new_symbol->templates, symbol_type->templates.size);

				for (size_t i = 0; i < symbol_type->templates.size; ++i) {
					ARENA_APPEND(&new_symbol->templates, solver_replace_templates(ARENA_GET(symbol_type->templates, i, ID)));
				}

				return new_symbol->info.type_id;
			}

			ID template_var_type = solver_get_template_type(symbol->name_id);

			// Not a template symbol?
			if (ID_IS_INVALID(template_var_type)) {
				return type_id;
			}

			return template_var_type;
		}
		case ID_FN_TYPE: {
			Fn_T fn_type = LOOKUP(type_id, Fn_T);

			ID fixed_arg_type = solver_replace_templates(fn_type.arg_type);
			ID fixed_ret_type = solver_replace_templates(fn_type.ret_type);

			// Nothing changed
			if (ID_IS_EQUAL(fixed_arg_type, fn_type.arg_type) && ID_IS_EQUAL(fixed_ret_type, fn_type.ret_type)) {
				return type_id;
			}

			Fn_T * new_fn_type = type_allocate(ID_FN_TYPE);
			new_fn_type->arg_type = fixed_arg_type;
			new_fn_type->ret_type = fixed_ret_type;
			new_fn_type->function_id = fn_type.function_id;

			return new_fn_type->info.type_id;
		}
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(type_id.type));
	}
}

ID solver_deflate_type(ID id) {
	switch (id.type) {
		case ID_INVALID_TYPE:
		case ID_NUMERIC_TYPE:
		case ID_VOID_TYPE: return id;
		case ID_PLACE_TYPE: {
			Place_T place = LOOKUP(id, Place_T);

			ID deflated_type_id = solver_deflate_type(place.basetype_id);
			if (ID_IS_EQUAL(place.basetype_id, deflated_type_id)) {
				return id;
			}

			Place_T * new_place = type_allocate(ID_PLACE_TYPE);
			new_place->basetype_id = deflated_type_id;
			new_place->is_mut = place.is_mut;
			return new_place->info.type_id;
		}
		case ID_REF_TYPE: {
			Ref_T ref = LOOKUP(id, Ref_T);

			ID deflated_type_id = solver_deflate_type(ref.basetype_id);
			if (ID_IS_EQUAL(ref.basetype_id, deflated_type_id)) {
				return id;
			}

			Ref_T * new_ref = type_allocate(ID_REF_TYPE);
			new_ref->basetype_id = deflated_type_id;
			new_ref->depth = ref.depth;
			new_ref->is_mut = ref.is_mut;
			return new_ref->info.type_id;
		}
		case ID_ARRAY_TYPE: {
			Array_T arr = LOOKUP(id, Array_T);

			ID deflated_type_id = solver_deflate_type(arr.basetype_id);
			if (ID_IS_EQUAL(arr.basetype_id, deflated_type_id)) {
				return id;
			}

			Array_T * new_arr = type_allocate(ID_ARRAY_TYPE);
			new_arr->basetype_id = deflated_type_id;
			new_arr->size = arr.size;
			return new_arr->info.type_id;
		}
		case ID_FN_TYPE: {
			Fn_T fn = LOOKUP(id, Fn_T);

			ID deflated_arg_type_id = solver_deflate_type(fn.arg_type);
			ID deflated_ret_type_id = solver_deflate_type(fn.ret_type);

			if (ID_IS_EQUAL(fn.arg_type, deflated_arg_type_id) && ID_IS_EQUAL(fn.ret_type, deflated_ret_type_id)) {
				return id;
			}

			Fn_T * new_fn = type_allocate(ID_FN_TYPE);
			new_fn->arg_type = deflated_arg_type_id;
			new_fn->ret_type = deflated_ret_type_id;
			new_fn->function_id = fn.function_id;
			return new_fn->info.type_id;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol = LOOKUP(id, Symbol_T);

			char create_new_flag = 0;
			ID templates[symbol.templates.size];
			for (size_t i = 0; i < symbol.templates.size; ++i) {
				ID template_id = ARENA_GET(symbol.templates, i, ID);
				templates[i] = solver_deflate_type(template_id);
				if (!ID_IS_EQUAL(templates[i], template_id)) {
					create_new_flag = 1;
				}
			}

			if (!create_new_flag) {
				return id;
			}

			Symbol_T * new_symbol = type_allocate(ID_SYMBOL_TYPE);
			new_symbol->symbol_id = symbol.symbol_id;
			
			arena_grow(&new_symbol->templates, symbol.templates.size);
			for (size_t i = 0; i < symbol.templates.size; ++i) {
				ARENA_APPEND(&new_symbol->templates, templates[i]);
			}

			return new_symbol->info.type_id;
		}
		case ID_TUPLE_TYPE: {
			Tuple_T tuple = LOOKUP(id, Tuple_T);

			char create_new_flag = 0;
			ID templates[tuple.types.size];
			for (size_t i = 0; i < tuple.types.size; ++i) {
				ID template_id = ARENA_GET(tuple.types, i, ID);
				templates[i] = solver_deflate_type(template_id);
				if (!ID_IS_EQUAL(templates[i], template_id)) {
					create_new_flag = 1;
				}
			}

			if (!create_new_flag) {
				return id;
			}

			Tuple_T * new_tuple = type_allocate(ID_TUPLE_TYPE);
			arena_grow(&new_tuple->types, tuple.types.size);
			for (size_t i = 0; i < tuple.types.size; ++i) {
				ARENA_APPEND(&new_tuple->types, templates[i]);
			}

			return new_tuple->info.type_id;
		}
		case ID_EXISTENIAL: {
			Existential existential = LOOKUP(id, Existential);

			if (ID_IS_INVALID(existential.solved_type)) {
				return id;
			}

			return solver_deflate_type(existential.solved_type);
		}
		default: FATAL("Unimplemented: {s}", id_type_to_string(id.type));
	}
}

#define MARKER_COUNT_GEN_SET(ENUM, _, TYPE, ...) marker->MARKER_COUNT_GET(TYPE) = registry_manager_get().TYPE.entries.item_count;
void solver_type_init(enum id_type type, void * ref) {
	switch (type) {
		case ID_TERM_VAR: {
			((TermVar *) ref)->symbol_id = INVALID_ID;
			((TermVar *) ref)->type = INVALID_ID;
		} break;
		case ID_EXISTENIAL: {
			((Existential *) ref)->solved_type = INVALID_ID;
		} break;
		case ID_TEMPLATE: {
			((Template *) ref)->name_id = INVALID_ID;
			((Template *) ref)->type_id = INVALID_ID;
		} break;
		case ID_MARKER: {
			Marker * marker = ref;
			TYPE_CHECKING_CONTEXT_KINDS(MARKER_COUNT_GEN_SET);
		} break;
		case ID_TYPE_VAR: break;
		default: FATAL("Unimplemented type: {s}", id_type_to_string(type));
	}
}
