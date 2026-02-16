#include "checker/typing/solver.h"

#include "tables/registry_manager.h"
#include "checker/checker.h"

struct invalid_flow {
	ID constraint_id;
	DdNode * from, * to;
};

void solver_add_new_flow(struct solver * ctx, ID from, ID to, DdNode * from_choice, DdNode * to_choice);
void solver_link_constraint(Constraint_TC * constraint);

void solver_initialize(struct solver * ctx) {
	ctx->worklist = DEQUE_INIT(ID);
	ctx->err_constraints = arena_init(sizeof(struct invalid_flow));
	ctx->resolver = dimension_resolver_init();
	ctx->constraints = kh_init(map_type_id_pair_to_constraint);

	LOOP_OVER_REGISTRY(Constraint_TC, c, {
		// println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));
		solver_link_constraint(c);

		if (!ID_IS(c->from, ID_TC_VARIABLE) || !ID_IS(c->to, ID_TC_VARIABLE)) {
			DEQUE_PUSH_BACK(ID, &ctx->worklist, c->constraint_id);
		}
	});
}

void solver_add_invalid(struct solver * ctx, Constraint_TC * constraint, DdNode * from_choice, DdNode * to_choice) {
	if (constraint->choice != NULL) {
		resolver_add_invalid_choice(&ctx->resolver, constraint->choice);
	}

	struct invalid_flow flow = { .constraint_id = constraint->constraint_id, .from = from_choice, .to = to_choice };
	ARENA_APPEND(&ctx->err_constraints, flow);
}

void solver_decompose(struct solver * ctx, Constraint_TC * c) {
	ASSERT1(!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE));

	println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));

	switch (c->to.type) {
		case ID_TC_SHAPE: {
			if (!ID_IS(c->from, ID_SYMBOL_TYPE)) {
				ID basetype_id;
				switch (c->from.type) {
					case ID_PLACE_TYPE: basetype_id = LOOKUP(c->from, Place_T).basetype_id; break;
					case ID_REF_TYPE: basetype_id = LOOKUP(c->from, Ref_T).basetype_id; break;
					default:
					  FATAL("Shape constraint on non symbol type: {s}", type_to_str(c->from));
				}

				solver_add_new_flow(ctx, basetype_id, c->to, NULL, c->choice);
				return;
			} 

			Symbol_T symbol = LOOKUP(c->from, Symbol_T);
			Shape_TC * shape = lookup(c->to);
			ID member_name_id = ast_get_interner_id(shape->member_id);

			if (!check_has_member(symbol.symbol_id, member_name_id)) {
				FATAL("{s} does not have member \"{s}\"", 
						interner_lookup_str(ast_get_interner_id(symbol.symbol_id))._ptr,
						interner_lookup_str(ast_get_interner_id(shape->member_id))._ptr
					);
			}

			return;
		}
		case ID_TC_DIMENSION: {
			Dimension_TC * dimension = lookup(c->to);

			const ID dimension_id = c->to;
			Arena candidates = dimension->candidates;
			for (size_t i = 0; i < candidates.size; ++i) {
				ID candidate_id = ARENA_GET(candidates, i, ID);
				DdNode * choice = dimension_get_choice(ctx->resolver, dimension_id, i);

				Arena temp_templates = {0};
				generate_template_constraints(candidate_id, &temp_templates);

				if (ID_IS(candidate_id, ID_AST_FUNCTION)) {
					a_function function = LOOKUP(candidate_id, a_function);
					ID to_type = replace_templates_in_type_with_template_variables(function.type, temp_templates, 0);
					solver_add_new_flow(ctx, to_type, c->from, choice, c->choice);
				}

				arena_free(temp_templates);
			}

			return;
		} break;
		case ID_TC_CAST: {
			Cast_TC * cast = lookup(c->to);
			Dimension_TC * dimension = lookup(cast->dimension_id);

			const ID dimension_id = cast->dimension_id, from_id = c->from, cast_variable_id = cast->variable_id;
			Arena candidates = dimension->candidates;
			for (size_t i = 0; i < candidates.size; ++i) {
				ID candidate_id = ARENA_GET(candidates, i, ID);
				DdNode * choice = dimension_get_choice(ctx->resolver, dimension_id, i);

				Arena temp_templates = {0};
				generate_template_constraints(candidate_id, &temp_templates);

				ASSERT1(ID_IS(candidate_id, ID_AST_IMPL));
				a_implementation impl = LOOKUP(candidate_id, a_implementation);
				ID impl_from_type_id = replace_templates_in_type_with_template_variables(ast_get_type_of(ARENA_GET(impl.templates, 0, ID)), temp_templates, 0);
				ID impl_to_type_id = replace_templates_in_type_with_template_variables(ast_get_type_of(ARENA_GET(impl.templates, 1, ID)), temp_templates, 0);
				// println("{s} -> {s} | {s}", type_to_str(impl_from_type_id), type_to_str(impl_to_type_id), type_to_str(ast_get_type_of(ARENA_GET(impl.templates, 1, ID))));

				solver_add_new_flow(ctx, from_id, impl_from_type_id, c->choice, choice);
				solver_add_new_flow(ctx, impl_to_type_id, cast_variable_id, choice, c->choice);

				arena_free(temp_templates);
			}

			return;
		} break;
	}

	if (c->from.type == c->to.type) {
		switch (c->from.type) {
			case ID_FN_TYPE: {
				Fn_T from = LOOKUP(c->from, Fn_T), to = LOOKUP(c->to, Fn_T);
				ASSERT1(ID_IS(from.arg_type, ID_TUPLE_TYPE));
				ASSERT1(ID_IS(to.arg_type, ID_TUPLE_TYPE));

				Tuple_T from_args_type = LOOKUP(from.arg_type, Tuple_T);
				Tuple_T to_args_type = LOOKUP(to.arg_type, Tuple_T);

				if (from_args_type.types.size != to_args_type.types.size) {
					FATAL("Incompatible argument counts");
				}

				DdNode * choice = c->choice;
				const size_t arg_count = from_args_type.types.size;
				for (size_t i = 0; i < arg_count; ++i) {
					ID to_type = ARENA_GET(to_args_type.types, i, ID);
					ID from_type = ARENA_GET(from_args_type.types, i, ID);

					solver_add_new_flow(ctx, to_type, from_type, choice, NULL);
				}

				// These are supposed to be swapped; the function return value flows into the called call-site return value
				solver_add_new_flow(ctx, from.ret_type, to.ret_type, NULL, choice);
				return;
			} break;
			case ID_PLACE_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Place_T * from = lookup(c->from), * to = lookup(c->to);
				solver_add_new_flow(ctx, from->basetype_id, to->basetype_id, c->choice, NULL);

				if (from->is_mut) {
					solver_add_new_flow(ctx, to->basetype_id, from->basetype_id, c->choice, NULL);
				}

				return;
			} break;
			case ID_REF_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Ref_T * from = lookup(c->from), * to = lookup(c->to);
				solver_add_new_flow(ctx, from->basetype_id, to->basetype_id, c->choice, NULL);
				return;
			} break;
			case ID_SYMBOL_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Symbol_T from_symbol_type = LOOKUP(c->from, Symbol_T), to_symbol_type = LOOKUP(c->to, Symbol_T);
				a_symbol from_symbol = LOOKUP(from_symbol_type.symbol_id, a_symbol), to_symbol = LOOKUP(to_symbol_type.symbol_id, a_symbol);

				for (size_t i = 0; i < from_symbol_type.templates.size; ++i) {
					ID from_symbol_template = ARENA_GET(from_symbol_type.templates, i, ID);
					ID to_symbol_template = ARENA_GET(to_symbol_type.templates, i, ID);
					solver_add_new_flow(ctx, from_symbol_template, to_symbol_template, c->choice, NULL);
				}

				return;
			}
			case ID_NUMERIC_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}
				return;
			} break;
			default:
				FATAL("Unimplemented type id: {s}", id_type_to_string(c->from.type));
		}
	}

	solver_add_invalid(ctx, c, NULL, NULL);
}

void solver_process_worklist(struct solver * ctx) {
	while (ctx->worklist.size > 0) {
		const struct registry_manager manager = registry_manager_get();
		ID constraint_id = DEQUE_FRONT(ID, &ctx->worklist);
		DEQUE_POP_FRONT(ID, &ctx->worklist);

		Constraint_TC * c = lookup(constraint_id);

		// Propogate forward:
		// Turn FROM -> To and To -> Next
		// Into: From -> Next (Transitive property)
		if (ID_IS(c->to, ID_TC_VARIABLE)) {
			Variable_TC * to_var = lookup(c->to);
			// println("Forward: {s} <: {s}", type_to_str(c->from), type_to_str(c->to));

			ID next_constraint_id = to_var->upper_bound;
			while (!ID_IS_INVALID(next_constraint_id)) {
				Constraint_TC * outgoing = lookup(next_constraint_id);
				solver_add_new_flow(ctx, c->from, outgoing->to, c->choice, outgoing->choice);
				next_constraint_id = outgoing->prev_upper_bound_for_from;
			}
		}

		// Propogate backward:
		// Prev -> From and From -> To
		// Into: Prev -> To (Transitive property)
		if (ID_IS(c->from, ID_TC_VARIABLE)) {
			Variable_TC * from_var = lookup(c->from);
			// println("Backward: {s} <: {s}", type_to_str(c->from), type_to_str(c->to));

			ID prev_constraint_id = from_var->lower_bound;
			while (!ID_IS_INVALID(prev_constraint_id)) {
				Constraint_TC * incoming = lookup(prev_constraint_id);
				solver_add_new_flow(ctx, incoming->from, c->to, incoming->choice, c->choice);
				prev_constraint_id = incoming->prev_lower_bound_for_to;
			}
		}

		// If neither side is a variable then check if it makes sense
		if (!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE)) {
			solver_decompose(ctx, c);
		} 
	}

	println("Dimensions: {i}", registry_manager_get().Dimension_TC.entries.item_count);
	println("Errors gathered: {i}", ctx->err_constraints.size);
	
	resolver_print_possibilities(ctx->resolver);
}

/*
0: 1? <: 2? [prev_lower_to: NULL, prev_upper_from: NULL]	| 1? [upper: 0], 2? [lower: 0]
1: 2? <: 3? [prev_lower_to: NULL, prev_upper_from: NULL]	| 2? [lower: 0, upper: 1], 3? [lower: 1]

2: 1? <: 3? [prev_lower_to: 1, prev_upper_from: 0]			| 1? [upper: 2], 3? [lower: 2]

3: i32 <: 1? [prev_lower_to: NULL, prev_upper_from: NULL]	| 1? [lower: 3, upper: 2]

4: i32 <: 3? [prev_lower_to: 2, prev_upper_from: NULL]		| 3? [lower: 4]
5: i32 <: 2? [prev_lower_to: 0, prev_upper_from: NULL]		| 2? [lower: 5, upper: 1]

6: i32 <: 3? [prev_lower_to: 4, prev_upper_from: NULL]		| 3? [lower: 6]
*/

void solver_link_constraint(Constraint_TC * constraint) {
	if (ID_IS(constraint->from, ID_TC_VARIABLE)) {
		Variable_TC * var = lookup(constraint->from);

		constraint->prev_upper_bound_for_from = var->upper_bound;
		var->upper_bound = constraint->constraint_id;
	} else {
		constraint->prev_upper_bound_for_from = INVALID_ID;
	}

	if (ID_IS(constraint->to, ID_TC_VARIABLE)) {
		Variable_TC * var = lookup(constraint->to);

		constraint->prev_lower_bound_for_to = var->lower_bound;
		var->lower_bound = constraint->constraint_id;
	} else {
		constraint->prev_lower_bound_for_to = INVALID_ID;
	}
}

ID solver_lookup_existing_constraint(struct solver * ctx, ID from, ID to) {
	khint_t k = kh_get(map_type_id_pair_to_constraint, &ctx->constraints, TYPE_IDS_TO_CONSTRAINT_PAIR(from, to));

	if (k == kh_end(&ctx->constraints)) {
		return INVALID_ID;
	}

	return kh_value(&ctx->constraints, k);
}

void solver_add_new_flow(struct solver * ctx, ID from, ID to, DdNode * from_choice, DdNode * to_choice) {
	DdNode * choice = NULL;

	if (from_choice != NULL && to_choice != NULL) {
		choice = Cudd_bddAnd(ctx->resolver.manager, from_choice, to_choice);
		Cudd_Ref(choice);
	} else if (from_choice != NULL) {
		choice = from_choice;
	} else if (to_choice != NULL) {
		choice = to_choice;
	}

	ID constraint_id = solver_lookup_existing_constraint(ctx, from, to);
	Constraint_TC * constraint;

	if (!ID_IS_INVALID(constraint_id)) {
		ASSERT1(ID_IS(constraint_id, ID_TC_CONSTRAINT));
		constraint = lookup(constraint_id);

		ASSERT1(type_check_deep_equal(constraint->from, from) && type_check_deep_equal(constraint->to, to));
		// println("Already exists ({s} <: {s}): {s} <: {s}", type_to_str(from), type_to_str(to), type_to_str(constraint->from), type_to_str(constraint->to));

		if (constraint->choice == NULL) {
			Cudd_Ref(choice);
			constraint->choice = choice;
		} else {
			if (constraint->choice == choice) {
				return;
			}

			DdNode * merged_choice = Cudd_bddOr(ctx->resolver.manager, constraint->choice, choice);
			Cudd_Ref(merged_choice);

			if (merged_choice == constraint->choice) {
				Cudd_RecursiveDeref(ctx->resolver.manager, merged_choice);
				return;
			}

			Cudd_RecursiveDeref(ctx->resolver.manager, constraint->choice);
			constraint->choice = merged_choice;
		}
	} else {
		constraint = tc_allocate(ID_TC_CONSTRAINT);
		constraint->from = from;
		constraint->to = to;
		constraint->choice = choice;

		println("\t{s} <: {s}", type_to_str(constraint->from), type_to_str(constraint->to));

		int ret_code;
		khint_t k = kh_put(map_type_id_pair_to_constraint, &ctx->constraints, TYPE_IDS_TO_CONSTRAINT_PAIR(from, to), &ret_code);
		ASSERT1(ret_code != KH_PUT_ALREADY_PRESENT);
		ASSERT(ret_code == KH_PUT_SUCCESS, "Error code: {i}", ret_code);

		kh_value(&ctx->constraints, k) = constraint->constraint_id;

		solver_link_constraint(constraint);
		constraint_id = constraint->constraint_id;
	}

	DEQUE_PUSH_BACK(ID, &ctx->worklist, constraint_id);

	if (constraint->choice == Cudd_ReadLogicZero(ctx->resolver.manager)) {
		solver_add_invalid(ctx, constraint, from_choice, to_choice);
	}
}
