#include "checker/typing/gathering.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"
#include "checker/symbol.h"

ID find_template(const Arena templates, ID name_id) {
	ASSERT1(ID_IS(name_id, ID_INTERNER));
	for (size_t i = 0; i < templates.size; ++i) {
		struct template_variable t_var = ARENA_GET(templates, i, struct template_variable);

		if (id_is_equal(t_var.name_id, name_id)) {
			return t_var.variable_id;
		}
	}

	return INVALID_ID;
}

ID replace_templates_in_type_with_template_variables(ID type_id, const Arena templates) {
	if (templates.size == 0) {
		return type_id;
	}

	switch (type_id.type) {
		case ID_NUMERIC_TYPE:
			return type_id;
		case ID_TUPLE_TYPE: {
			Tuple_T tuple = LOOKUP(type_id, Tuple_T);

			ID fixed_type_id = INVALID_ID;
			size_t failed_index = -1;

			for (size_t i = 0; i < tuple.types.size; ++i) {
				ID type_id = ARENA_GET(tuple.types, i, ID);
				fixed_type_id = replace_templates_in_type_with_template_variables(type_id, templates);
				if (!id_is_equal(type_id, fixed_type_id)) {
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
				fixed_type_id = replace_templates_in_type_with_template_variables(type_id, templates);
				ARENA_APPEND(&new_tuple->types, fixed_type_id);
			}

			return new_tuple->info.type_id;
		}
		case ID_ARRAY_TYPE: {
			Array_T array = LOOKUP(type_id, Array_T);
			ID fixed_type = replace_templates_in_type_with_template_variables(array.basetype_id, templates);
			if (id_is_equal(fixed_type, array.basetype_id)) {
				return type_id; // Nothing changed
			}

			Array_T * new_array = type_allocate(ID_ARRAY_TYPE);
			new_array->basetype_id = fixed_type;
			new_array->size = array.size;

			return new_array->info.type_id;
		}
		case ID_REF_TYPE: {
			Ref_T ref = LOOKUP(type_id, Ref_T);
			ID fixed_type = replace_templates_in_type_with_template_variables(ref.basetype_id, templates);
			if (id_is_equal(fixed_type, ref.basetype_id)) {
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
			ID fixed_type = replace_templates_in_type_with_template_variables(place.basetype_id, templates);
			if (id_is_equal(place.basetype_id, fixed_type)) {
				return type_id; // Nothing changed
			}

			Place_T * new_place = type_allocate(ID_PLACE_TYPE);
			new_place->basetype_id = fixed_type;
			new_place->is_mut = place.is_mut;

			return new_place->info.type_id;
		}
		case ID_SYMBOL_TYPE: {
			Symbol_T * symbol_type = lookup(type_id);

			if (symbol_type->templates.size != 0) {
				Symbol_T * new_symbol = type_allocate(ID_SYMBOL_TYPE);
				new_symbol->symbol_id = symbol_type->symbol_id;
				new_symbol->templates = arena_init(sizeof(ID));
				arena_grow(&new_symbol->templates, symbol_type->templates.size);

				for (size_t i = 0; i < symbol_type->templates.size; ++i) {
					ARENA_APPEND(&new_symbol->templates, replace_templates_in_type_with_template_variables(ARENA_GET(symbol_type->templates, i, ID), templates));
				}

				return new_symbol->info.type_id;
			}

			a_symbol symbol = LOOKUP(symbol_type->symbol_id, a_symbol);
			ID template_var_type = find_template(templates, symbol.name_id);

			// Not a template symbol?
			if (ID_IS_INVALID(template_var_type)) {
				return type_id;
			}

			return template_var_type;
		}
		case ID_FN_TYPE: {
			Fn_T fn_type = LOOKUP(type_id, Fn_T);

			ID fixed_arg_type = replace_templates_in_type_with_template_variables(fn_type.arg_type, templates);
			ID fixed_ret_type = replace_templates_in_type_with_template_variables(fn_type.ret_type, templates);

			// Nothing changed
			if (id_is_equal(fixed_arg_type, fn_type.arg_type) && id_is_equal(fixed_ret_type, fn_type.ret_type)) {
				return type_id;
			}

			Fn_T * new_fn_type = type_allocate(ID_FN_TYPE);
			new_fn_type->arg_type = fixed_arg_type;
			new_fn_type->ret_type = fixed_ret_type;
			new_fn_type->function_id = fn_type.function_id;

			return new_fn_type->info.type_id;
		}
		case ID_TC_VARIABLE: {
			return type_id;
		}
		case ID_VOID_TYPE:
			return type_id;
		default:
			FATAL("Invalid ID type: {s}", id_type_to_string(type_id.type));
	}
}

void populate_template_list(Arena arena, Arena * templates) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID template_id = ARENA_GET(arena, i, ID);
		ASSERT1(ID_IS(template_id, ID_AST_SYMBOL));
		a_symbol template_symbol = LOOKUP(template_id, a_symbol);
		ASSERT1(template_symbol.name_ids.size == 1);

		// If no constraint is specified
		if (ID_IS_INVALID(template_symbol.node_id)) {
			Variable_TC * variable = tc_allocate(ID_TC_VARIABLE);
			struct template_variable new_var = { .variable_id = variable->variable_id, .name_id = template_symbol.name_id };
			ARENA_APPEND(templates, new_var);
			continue;
		}

		ASSERT1(ID_IS(template_symbol.node_id, ID_AST_VARIABLE));
		a_variable t_s_var = LOOKUP(template_symbol.node_id, a_variable);
		ASSERT1(!ID_IS_INVALID(t_s_var.type_id));

		struct template_variable new_var = { .variable_id = replace_templates_in_type_with_template_variables(t_s_var.type_id, *templates), .name_id = template_symbol.name_id };
		ARENA_APPEND(templates, new_var);
	}
}

void generate_trait_implementation_usage_constraints(ID trait_id, Arena passed_template_types, Arena templates) {
	ASSERT1(ID_IS(trait_id, ID_AST_TRAIT));
	a_trait * trait = lookup(trait_id);
	ASSERT1(trait->templates.size == passed_template_types.size); // should've been checked in pre-checker

	Variable_TC * variable = tc_allocate(ID_TC_VARIABLE);

	Dimension_TC * dimension = tc_allocate(ID_TC_DIMENSION);
	dimension->candidates = trait->implementations;

	Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
	constraint->from = variable->variable_id;
	constraint->to = dimension->dimension_id;
}

void generate_where_constraints(Arena arena, Arena templates) {
	for (size_t i = 0; i < arena.size; ++i) {
		ID constraint = ARENA_GET(arena, i, ID);
		ASSERT1(ID_IS(constraint, ID_SYMBOL_TYPE));
		Symbol_T * symbol_type = lookup(constraint);

		ASSERT1(ID_IS(symbol_type->symbol_id, ID_AST_SYMBOL));
		a_symbol * symbol = lookup(symbol_type->symbol_id);

		if (ID_IS_INVALID(symbol->node_id)) {
			ID found = qualify_symbol(symbol, ID_AST_TRAIT);
			ASSERT1(!ID_IS_INVALID(found));
		}

		ASSERT1(ID_IS(symbol->node_id, ID_AST_TRAIT));
		generate_trait_implementation_usage_constraints(symbol->node_id, symbol_type->templates, templates);
	}
}

void generate_template_constraints(ID node_id, Arena * templates) {
	if (templates->arena == NULL) {
		*templates = arena_init(sizeof(struct template_variable));
	}

    switch (node_id.type) {
		case ID_AST_VARIABLE: {
			a_variable * variable = lookup(node_id);
			if  (!ID_IS(variable->info.scope_id, ID_AST_MODULE)) {
				generate_template_constraints(variable->info.scope_id, templates);
			}
		} break;
        case ID_AST_FUNCTION: {
            a_function function = LOOKUP(node_id, a_function);
			if (!ID_IS(function.info.scope_id, ID_AST_MODULE)) {
				generate_template_constraints(function.info.scope_id, templates);
			}

			populate_template_list(function.templates, templates);
        } break;
        case ID_AST_STRUCT: {
            a_structure _struct = LOOKUP(node_id, a_structure);

			populate_template_list(_struct.templates, templates);
        } break;
        case ID_AST_IMPL: {
            a_implementation impl = LOOKUP(node_id, a_implementation);

			populate_template_list(impl.generics, templates);
			populate_template_list(impl.templates, templates);
			generate_where_constraints(impl.where, *templates);
        } break;
		case ID_AST_SCOPE: {
			a_scope * scope = lookup(node_id);
			generate_template_constraints(scope->info.scope_id, templates);
		} break;
        default:
            FATAL("Invalid id type: {s}", id_type_to_string(node_id.type));
    }
}
