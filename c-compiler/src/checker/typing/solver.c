#include "checker/typing/solver.h"

#include "parser/types.h"
#include "tables/registry_manager.h"
#include "checker/typing/gathering.h"
#include "checker/typing/groups.h"
#include "checker/symbol.h"

// void solver_initialize(struct solver * ctx) {
//         ctx->worklist = DEQUE_INIT(ID);
//         ctx->err_constraints = arena_init(sizeof(struct invalid_flow));
//         ctx->resolver = dimension_resolver_init();
//         ctx->constraints = kh_init(map_type_id_pair_to_constraint);
//
//         LOOP_OVER_REGISTRY(Constraint_TC, c, {
//                 // println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));
//                 solver_link_constraint(c);
//
//                 if (!ID_IS(c->from, ID_TC_VARIABLE) || !ID_IS(c->to, ID_TC_VARIABLE)) {
//                         DEQUE_PUSH_BACK(ID, &ctx->worklist, c->constraint_id);
//                 }
//         });
// }

void generate_template_constraints(Solver * solver, ID node_id, Arena * templates, DdNode * parent_choice);

void solver_initialize(Solver * solver) {
	solver->resolver = dimension_resolver_init();
	// solver->err_constraints = arena_init(sizeof(struct invalid_flow));
}

char solver_decompose_same(Solver * solver, ID id1, ID id2, DdNode * world) {
	ASSERT1(id1.type == id2.type);

	println("{s} == {s}", type_to_str(id1), type_to_str(id2));

	switch (id1.type) {
		case ID_TUPLE_TYPE: {
			Tuple_T tuple1 = LOOKUP(id1, Tuple_T), tuple2 = LOOKUP(id2, Tuple_T);
			if (tuple1.types.size != tuple2.types.size) {
				ERROR("Incompatiable tuple sizes");
				return 0;
			}
			
			for (size_t i = 0; i < tuple1.types.size; ++i) {
				solver_unify(solver, ARENA_GET(tuple1.types, i, ID), ARENA_GET(tuple2.types, i, ID), world);
			}
		} break;
		case ID_FN_TYPE: {
			Fn_T fn1 = LOOKUP(id1, Fn_T), fn2 = LOOKUP(id2, Fn_T);
			
			solver_decompose_same(solver, fn1.arg_type, fn2.arg_type, world);
			solver_unify(solver, fn1.ret_type, fn2.ret_type, world);
		} break;
		case ID_PLACE_TYPE: {
			Place_T place1 = LOOKUP(id1, Place_T), place2 = LOOKUP(id2, Place_T);
			
			if (place1.is_mut != place2.is_mut) {
				ERROR("Invalid place mutability");
				return 0;
			}

			solver_unify(solver, place1.basetype_id, place2.basetype_id, world);
		} break;
		case ID_NUMERIC_TYPE: {
			Numeric_T num1 = LOOKUP(id1, Numeric_T), num2 = LOOKUP(id2, Numeric_T);
			if (!(num1.type == num2.type && num1.width == num2.width)) {
				ERROR("Not equivalent numeric types: {s} != {s}", type_to_str(id1), type_to_str(id2));
				return 0;
			}

			return 1;
		} break;
		default: FATAL("Unimplemented type: {s}", id_type_to_string(id1.type));
	}

	return 1;
}

static inline char solver_decompose_dimension(Solver * solver, ID dimension_id, ID other_id, DdNode * world) {
	Dimension_TC dimension = LOOKUP(dimension_id, Dimension_TC);
	
	Arena candidates = dimension.candidates;
	for (size_t i = 0; i < candidates.size; ++i) {
		ID candidate_id = ARENA_GET(candidates, i, ID);
		DdNode * choice = dimension_get_choice(solver->resolver, dimension_id, i);

		Arena temp_templates = {0};
		generate_template_constraints(solver, candidate_id, &temp_templates, choice);

		switch (candidate_id.type) {
			case ID_AST_FUNCTION: {
				a_function function = LOOKUP(candidate_id, a_function);
				ID to_type = replace_templates_in_type_with_template_variables(solver, function.type, temp_templates);
				println("func: {s} == {s}", type_to_str(other_id), type_to_str(to_type));
				solver_decompose_same(solver, other_id, to_type, choice);
			} break;
			case ID_AST_IMPL: {
				a_implementation impl = LOOKUP(candidate_id, a_implementation);
				const Tuple_T tuple_type = LOOKUP(other_id, Tuple_T);
				ASSERT1(tuple_type.types.size == impl.templates.size);

				for (size_t i = 0; i < impl.templates.size; ++i) {
					ID impl_replaced_type = replace_templates_in_type_with_template_variables(solver, ast_get_type_of(ARENA_GET(impl.templates, i, ID)), temp_templates);
					solver_unify(solver, ARENA_GET(tuple_type.types, i, ID), impl_replaced_type, choice);
				}
			} break;
			default: FATAL("Unhandled candidate type: {s}", id_type_to_string(candidate_id.type));
		}

		arena_free(temp_templates);
	}

	return 1;
}

static inline char solver_decompose_cast(Solver * solver, ID cast_id, ID other_id, DdNode * world) {
	Cast_TC * cast = lookup(cast_id);
	Dimension_TC * dimension = lookup(cast->dimension_id);

	const ID dimension_id = cast->dimension_id, cast_variable_id = cast->variable_id;
	Arena candidates = dimension->candidates;
	for (size_t i = 0; i < candidates.size; ++i) {
		ID candidate_id = ARENA_GET(candidates, i, ID);
		DdNode * choice = dimension_get_choice(solver->resolver, dimension_id, i);

		Arena temp_templates = {0};
		generate_template_constraints(solver, candidate_id, &temp_templates, choice);

		ASSERT1(ID_IS(candidate_id, ID_AST_IMPL));
		a_implementation impl = LOOKUP(candidate_id, a_implementation);
		ID impl_from_type_id = replace_templates_in_type_with_template_variables(solver, ast_get_type_of(ARENA_GET(impl.templates, 0, ID)), temp_templates);
		ID impl_to_type_id = replace_templates_in_type_with_template_variables(solver, ast_get_type_of(ARENA_GET(impl.templates, 1, ID)), temp_templates);
		// println("{s} -> {s} | {s}", type_to_str(impl_from_type_id), type_to_str(impl_to_type_id), type_to_str(ast_get_type_of(ARENA_GET(impl.templates, 1, ID))));

		solver_unify(solver, other_id, impl_from_type_id, choice);
		solver_unify(solver, impl_to_type_id, cast_variable_id, choice);

		arena_free(temp_templates);
	}

	return 1;
}

char solver_decompose(Solver * solver, ID id1, ID id2, DdNode * world) {
	ASSERT1(!ID_IS(id1, ID_TC_VARIABLE));
	ASSERT1(!ID_IS(id2, ID_TC_VARIABLE));

	if (id1.type == id2.type) {
		return solver_decompose_same(solver, id1, id2, world);
	}

	println("{s} == {s}", type_to_str(id1), type_to_str(id2));

	switch (id1.type) {
		case ID_TC_DIMENSION: return solver_decompose_dimension(solver, id1, id2, world);
		case ID_TC_CAST: return solver_decompose_cast(solver, id1, id2, world);
		default: break;
	}

	switch (id2.type) {
		case ID_TC_DIMENSION: return solver_decompose_dimension(solver, id2, id1, world);
		case ID_TC_CAST: return solver_decompose_cast(solver, id2, id1, world);
		default: break;
	}
	
	switch (id1.type) {
		case ID_TUPLE_TYPE:
		case ID_PLACE_TYPE:
		case ID_NUMERIC_TYPE:
			ERROR("Incompatible types: {s} != {s}", type_to_str(id1), type_to_str(id2));
			return 0;
		default: FATAL("Unimplemented type: {s}", id_type_to_string(id1.type));
	}

	return 1;
}

void solver_unify(struct solver * solver, ID id1, ID id2, DdNode * world) {
	ASSERT1(!ID_IS_INVALID(id1));
	ASSERT1(!ID_IS_INVALID(id2));

	if (!ID_IS(id1, ID_TC_VARIABLE) && !ID_IS(id2, ID_TC_VARIABLE)) {
		// Both are requirements so decompose into less complex types

		if (!solver_decompose(solver, id1, id2, world)) {
			solver_add_invalid_world(solver, world);
		}

		return;
	} else if (!ID_IS(id2, ID_TC_VARIABLE)) {
		// ID1 is a variable ID2 is a requirement
		return solver_add_variable_group_requirement(solver, id1, id2, world);
	} else if (!ID_IS(id1, ID_TC_VARIABLE)) {
		// ID2 is a variable ID1 is a requirement
		return solver_add_variable_group_requirement(solver, id2, id1, world);
	}

	// Two variables
	ASSERT1(ID_IS(id1, ID_TC_VARIABLE));
	ASSERT1(ID_IS(id2, ID_TC_VARIABLE));
	Variable_TC * const var1 = lookup(id1);
	Variable_TC * const var2 = lookup(id2);
	
	Group_TC * group1;
	Group_TC * group2;
	
	if (ID_IS_INVALID(var1->group_id) && ID_IS_INVALID(var2->group_id)) {
		group1 = group2 = tc_allocate(ID_TC_GROUP);
		var1->group_id = var2->group_id = group1->group_id;
	} else if (ID_IS_INVALID(var1->group_id)) {
		group1 = group2 = group_find_root_group(var2->group_id);
		var1->group_id = group1->group_id;
	} else if (ID_IS_INVALID(var2->group_id)) {
		group1 = group2 = group_find_root_group(var1->group_id);
		var2->group_id = group2->group_id;
	} else {
		group1 = group_find_root_group(var1->group_id);
		group2 = group_find_root_group(var2->group_id);
	}

	if (ID_IS_EQUAL(group1->group_id, group2->group_id)) {
		return;
	}

	ASSERT1(ID_IS_INVALID(group1->parent_group_id));
	ASSERT1(ID_IS_INVALID(group2->parent_group_id));

	Group_TC * parent, * child;
	if (group1->rank < group2->rank) {
		parent = group2, child = group1;
	} else {
		parent = group1, child= group2;
	}

	child->parent_group_id = parent->group_id;
	child->rank = parent->rank + 1; // this shouldn't ever have to be used but good to add anyway

	if (ID_IS_INVALID(child->requirement)) {
		return;
	}

	if (ID_IS_INVALID(parent->requirement)) {
		parent->requirement = child->requirement;
		return;
	}

/*
*	parent: A -> B
*	extracted: NULL
*	child:	C -> D -> E
*
*	parent: C -> A -> B
*	extracted: C | A -> B
*	child: D -> E
*
*	parent: (A & D) -> B
*	extracted: C | (A & D) -> B
*	child: E
*	
*	parent: E -> (A & D) -> B
*	extracted: C -> E | (A & D) -> B
*
*	parent: C -> E -> (A & D) -> B
*/

	ID parent_start_requirment_id = parent->requirement;
	ID next_child_requirement_id = child->requirement;

	Requirement_TC * extracted = NULL;
	ID first_extracted = INVALID_ID;
	while (!ID_IS_INVALID(next_child_requirement_id)) {
		Requirement_TC * child_requirement = lookup(next_child_requirement_id);

		next_child_requirement_id = child_requirement->next_requirement;
		child_requirement->next_requirement = INVALID_ID;

		solver_add_group_requirement(solver, parent, child_requirement, world);
		
		// Didn't find a match and added child_requirement to front of parent's requirement list
		if (ID_IS_EQUAL(parent->requirement, child_requirement->requirement_id)) {
			// Move parent requirement back to before child was added
			parent->requirement = child_requirement->next_requirement;

			if (extracted == NULL) {
				first_extracted = child_requirement->requirement_id;
				extracted = child_requirement;
			} else {
				// Add child requirement as next in extracted
				extracted->next_requirement = child_requirement->requirement_id;
				extracted = child_requirement;
			}

			// Detach extracted from parent
			extracted->next_requirement = INVALID_ID;
		}
	}

	if (extracted != NULL) {
		extracted->next_requirement = parent->requirement;
		parent->requirement = first_extracted;
	}

	return;
}

void generate_trait_implementation_usage_constraints(ID trait_id, Arena passed_template_types, Arena templates, struct solver * ctx, DdNode * parent_choice) {
	ASSERT1(ID_IS(trait_id, ID_AST_TRAIT));
	ASSERT1(passed_template_types.size > 0);
	a_trait * trait = lookup(trait_id);
	ASSERT1(trait->templates.size == passed_template_types.size); // should've been checked in pre-checker

	Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE);
	arena_grow(&tuple->types, passed_template_types.size);

	for (size_t i = 0; i < passed_template_types.size; ++i) {
		ID template = ARENA_GET(passed_template_types, i, ID);
		ASSERT1(ID_IS(template, ID_SYMBOL_TYPE));
		Symbol_T symbol_type = LOOKUP(template, Symbol_T);
		ASSERT1(symbol_type.templates.size == 0);

		ID template_type = find_template(templates, ast_get_interner_id(symbol_type.symbol_id));
		ARENA_APPEND(&tuple->types, template_type);
	}

	Dimension_TC * dimension = tc_allocate(ID_TC_DIMENSION);
	dimension->candidates = trait->implementations;

	println("D{u}: {s}", dimension->dimension_id.id, interner_lookup_str(trait->name_id)._ptr);

	// if (ctx == NULL) {
	// 	Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
	// 	constraint->from = tuple->info.type_id;
	// 	constraint->to = dimension->dimension_id;
	// } else {
	// 	dimension_init_choices(&ctx->resolver, dimension, parent_choice);
	// 	solver_add_new_flow(ctx, tuple->info.type_id, dimension->dimension_id, NULL, NULL);
	// }
}

void generate_where_constraints(Arena arena, Arena templates, struct solver * ctx, DdNode * parent_choice) {
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
		generate_trait_implementation_usage_constraints(symbol->node_id, symbol_type->templates, templates, ctx, parent_choice);
	}
}

void generate_template_constraints(Solver * solver, ID node_id, Arena * templates, DdNode * parent_choice) {
	if (templates->arena == NULL) {
		*templates = arena_init(sizeof(struct template_variable));
	}

    switch (node_id.type) {
		case ID_AST_VARIABLE: {
			a_variable * variable = lookup(node_id);
			if  (!ID_IS(variable->info.scope_id, ID_AST_MODULE)) {
				generate_template_constraints(solver, variable->info.scope_id, templates, parent_choice);
			}
		} break;
        case ID_AST_FUNCTION: {
            a_function function = LOOKUP(node_id, a_function);
			if (!ID_IS(function.info.scope_id, ID_AST_MODULE)) {
				generate_template_constraints(solver, function.info.scope_id, templates, parent_choice);
			}

			populate_template_list_from_arena(solver, function.templates, templates);
			generate_where_constraints(function.where, *templates, solver, parent_choice);
        } break;
        case ID_AST_STRUCT: {
            a_structure _struct = LOOKUP(node_id, a_structure);

			populate_template_list_from_arena(solver, _struct.templates, templates);
        } break;
        case ID_AST_IMPL: {
            a_implementation impl = LOOKUP(node_id, a_implementation);

			populate_template_list_from_arena(solver, impl.generics, templates);
			populate_template_list_from_arena(solver, impl.templates, templates);
			generate_where_constraints(impl.where, *templates, solver, parent_choice);
        } break;
		case ID_AST_SCOPE: {
			a_scope * scope = lookup(node_id);
			generate_template_constraints(solver, scope->info.scope_id, templates, parent_choice);
		} break;
        default:
            FATAL("Invalid id type: {s}", id_type_to_string(node_id.type));
    }
}
