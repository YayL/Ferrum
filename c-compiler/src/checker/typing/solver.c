#include "checker/typing/solver.h"

#include "tables/registry_manager.h"
#include "checker/checker.h"

void solver_add_new_flows(struct solver * ctx, ID from, ID to, DdNode * choice);
void solver_link_constraint(Constraint_TC * constraint);

void solver_initialize(struct solver * ctx) {
	ctx->worklist = DEQUE_INIT(ID);
	ctx->err_constraints = arena_init(sizeof(ID));
	ctx->resolver = dimension_resolver_init();

	LOOP_OVER_REGISTRY(Constraint_TC, c, {
		/* println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to)); */
		solver_link_constraint(c);

		if (!ID_IS(c->from, ID_TC_VARIABLE) || !ID_IS(c->to, ID_TC_VARIABLE)) {
			DEQUE_PUSH_BACK(ID, &ctx->worklist, c->constraint_id);
		}
	});
}

void solver_decompose(struct solver * ctx, Constraint_TC * c) {
	ASSERT1(!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE));

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

				solver_add_new_flows(ctx, basetype_id, c->to, c->choice);
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
			dimension_init_choices(ctx->resolver, dimension);
		} break;
		case ID_SYMBOL_TYPE: {
			Symbol_T symbol_type = LOOKUP(c->to, Symbol_T);
			a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);
			ASSERT1(!ID_IS_INVALID(symbol.node_id));

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

					// These are supposed to be swapped; the call-site arguments flow into the called functions parameters
					solver_add_new_flows(ctx, to_type, from_type, choice);
				}

				solver_add_new_flows(ctx, from.ret_type, to.ret_type, choice);
				return;
			} break;
			case ID_PLACE_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Place_T * from = lookup(c->from), * to = lookup(c->to);
				solver_add_new_flows(ctx, from->basetype_id, to->basetype_id, c->choice);
				return;
			}
			case ID_REF_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Ref_T * from = lookup(c->from), * to = lookup(c->to);
				solver_add_new_flows(ctx, from->basetype_id, to->basetype_id, c->choice);
				return;
			}
			case ID_SYMBOL_TYPE: {
				if (!type_check_equal(c->from, c->to)) {
					break;
				}

				Symbol_T from_symbol_type = LOOKUP(c->from, Symbol_T), to_symbol_type = LOOKUP(c->to, Symbol_T);
				a_symbol from_symbol = LOOKUP(from_symbol_type.symbol_id, a_symbol), to_symbol = LOOKUP(to_symbol_type.symbol_id, a_symbol);

				for (size_t i = 0; i < from_symbol_type.templates.size; ++i) {
					ID from_symbol_template = ARENA_GET(from_symbol_type.templates, i, ID);
					ID to_symbol_template = ARENA_GET(to_symbol_type.templates, i, ID);
					solver_add_new_flows(ctx, from_symbol_template, to_symbol_template, c->choice);
				}

				return;
			}
			default:
				FATAL("Unimplemented type id: {s}", id_type_to_string(c->from.type));
		}
	}
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

			ID next_constraint_id = to_var->outgoing_head;
			while (!ID_IS_INVALID(next_constraint_id)) {
				Constraint_TC * outgoing = lookup(next_constraint_id);
				DdNode * merged_choice = Cudd_bddAnd(ctx->resolver.manager, c->choice, outgoing->choice);

				if (merged_choice != Cudd_ReadLogicZero(ctx->resolver.manager)) {
					solver_add_new_flows(ctx, c->from, outgoing->to, merged_choice);
				} else {
					Cudd_RecursiveDeref(ctx->resolver.manager, merged_choice);
				}

				next_constraint_id = outgoing->next_outgoing_for_from;
			}
		}

		// Propogate backward:
		// Prev -> From and From -> To
		// Into: Prev -> To (Transitive property)
		if (ID_IS(c->from, ID_TC_VARIABLE)) {
			Variable_TC * from_var = lookup(c->from);
			// println("Backward: {s} <: {s}", type_to_str(c->from), type_to_str(c->to));

			ID prev_constraint_id = from_var->incoming_head;
			while (!ID_IS_INVALID(prev_constraint_id)) {
				Constraint_TC * incoming = lookup(prev_constraint_id);
				DdNode * merged_choice = Cudd_bddAnd(ctx->resolver.manager, c->choice, incoming->choice);

				if (merged_choice != Cudd_ReadLogicZero(ctx->resolver.manager)) {
					solver_add_new_flows(ctx, incoming->from, c->to, merged_choice);
				} else {
					Cudd_RecursiveDeref(ctx->resolver.manager, merged_choice);
				}

				prev_constraint_id = incoming->next_incoming_for_to;
			}
		}

		// If neither side is a variable then check if it makes sense
		if (!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE)) {
			solver_decompose(ctx, c);
		} else if (ID_IS(c->from, ID_PLACE_TYPE)) {
			Place_T * place = lookup(c->from);
			if (!ID_IS(place->basetype_id, ID_TC_VARIABLE)) {
				solver_add_new_flows(ctx, place->basetype_id, c->to, c->choice);
			}
		}

		println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));
	}

	println("Dimensions: {i}", registry_manager_get().Dimension_TC.entries.item_count);
	println("Errors gathered: {i}", ctx->err_constraints.size);

	Arena errs = ctx->err_constraints;
	for (size_t i = 0; i < errs.size; ++i) {
		Constraint_TC constraint = LOOKUP(ARENA_GET(errs, i, ID), Constraint_TC);

		println("{i}) {s} <: {s}", i + 1, type_to_str(constraint.from), type_to_str(constraint.to));
	}
}

void solver_link_constraint(Constraint_TC * constraint) {
	if (ID_IS(constraint->from, ID_TC_VARIABLE)) {
		Variable_TC * var = lookup(constraint->from);

		constraint->next_outgoing_for_from = var->outgoing_head;
		var->outgoing_head = constraint->constraint_id;
	} else {
		constraint->next_outgoing_for_from = INVALID_ID;
	}

	if (ID_IS(constraint->to, ID_TC_VARIABLE)) {
		Variable_TC * var = lookup(constraint->to);

		constraint->next_incoming_for_to = var->incoming_head;
		var->incoming_head = constraint->constraint_id;
	} else {
		constraint->next_incoming_for_to = INVALID_ID;
	}
}

void solver_add_new_flows(struct solver * ctx, ID from, ID to, DdNode * choice) {
	if (id_is_equal(from, to)) {
		return;
	}

	Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
	constraint->from = from;
	constraint->to = to;
	constraint->choice = choice;

	solver_link_constraint(constraint);

	DEQUE_PUSH_BACK(ID, &ctx->worklist, constraint->constraint_id);
}
