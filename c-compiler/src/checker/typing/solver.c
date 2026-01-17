#include "checker/typing/solver.h"

#include "tables/registry_manager.h"

void solver_add_new_flows(struct solver * ctx, ID from, ID to, ID config_id);
void solver_link_constraint(Constraint_TC * constraint);
ID config_merge(ID config1_id, ID config2_id, char * is_impossible_flag);

void solver_initialize(struct solver * ctx) {
	ctx->worklist = DEQUE_INIT(ID);
	LOOP_OVER_REGISTRY(Constraint_TC, c, {
		solver_link_constraint(c);

		if (!ID_IS(c->from, ID_TC_VARIABLE) || !ID_IS(c->to, ID_TC_VARIABLE)) {
			DEQUE_PUSH_BACK(ID, &ctx->worklist, c->constraint_id);
		}
	});
}

void solver_decompose(struct solver * ctx, Constraint_TC * c) {
	ASSERT1(!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE));

	if (!ID_IS(c->from, ID_PLACE_TYPE) && ID_IS(c->to, ID_PLACE_TYPE)) {
		FATAL("A non-place can not flow into a place");
	}

	switch (c->from.type) {
		case ID_FN_TYPE: {
			if (!ID_IS(c->to, ID_FN_TYPE)) {
				break;
			}

			Fn_T from = LOOKUP(c->from, Fn_T), to = LOOKUP(c->to, Fn_T);
			ASSERT1(ID_IS(from.arg_type, ID_TUPLE_TYPE));
			ASSERT1(ID_IS(to.arg_type, ID_TUPLE_TYPE));

			Tuple_T from_args_type = LOOKUP(from.arg_type, Tuple_T);
			Tuple_T to_args_type = LOOKUP(to.arg_type, Tuple_T);

			if (from_args_type.types.size != to_args_type.types.size) {
				FATAL("Incompatible argument counts");
			}

			const ID config_id = c->config_id;
			const size_t arg_count = from_args_type.types.size;
			for (size_t i = 0; i < arg_count; ++i) {
				solver_add_new_flows(ctx, ARENA_GET(from_args_type.types, i, ID), ARENA_GET(to_args_type.types, i, ID), config_id);
			}
		} break;
		case ID_PLACE_TYPE: {
			ID to_type = c->to;
			if (ID_IS(c->to, ID_PLACE_TYPE)) {
				Place_T * place_type = lookup(c->to);
				to_type = place_type->basetype_id;
			}

			Place_T * from_place_type = lookup(c->from);
			solver_add_new_flows(ctx, from_place_type->basetype_id, to_type, c->config_id);
		} break;
		case ID_REF_TYPE:
		case ID_SYMBOL_TYPE:
		case ID_NUMERIC_TYPE: // println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));
		break;
		default:
			FATAL("Unimplemented from type: {s}", id_type_to_string(c->from.type));
	}
}

void solver_process_worklist(struct solver * ctx) {
	while (ctx->worklist.size > 0) {
		ID constraint_id = DEQUE_FRONT(ID, &ctx->worklist);
		DEQUE_POP_FRONT(ID, &ctx->worklist);

		Constraint_TC * c = lookup(constraint_id);
		// println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));

		// Propogate forward:
		// Turn FROM -> To and To -> Next
		// Into: From -> Next (Transitive property)
		if (ID_IS(c->to, ID_TC_VARIABLE)) {
			Variable_TC * to_var = lookup(c->to);
			// println("Forward: {s} <: {s}", type_to_str(c->from), type_to_str(c->to));

			ID next_constraint_id = to_var->outgoing_head;
			while (!ID_IS_INVALID(next_constraint_id)) {
				Constraint_TC * outgoing = lookup(next_constraint_id);
				char is_impossible = 0;
				ID new_config = config_merge(c->config_id, outgoing->config_id, &is_impossible);

				if (!is_impossible) {
					solver_add_new_flows(ctx, c->from, outgoing->to, new_config);
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
				char is_impossible = 0;
				ID new_config = config_merge(c->config_id, incoming->config_id, &is_impossible);

				if (!is_impossible) {
					solver_add_new_flows(ctx, incoming->from, c->to, new_config);
				}

				prev_constraint_id = incoming->next_incoming_for_to;
			}
		}

		// If neither side is a variable then check if it makes sense
		if (!ID_IS(c->from, ID_TC_VARIABLE) && !ID_IS(c->to, ID_TC_VARIABLE)) {
			// println("{s} <: {s}", type_to_str(c->from), type_to_str(c->to));

			solver_decompose(ctx, c);
		}
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

void solver_add_new_flows(struct solver * ctx, ID from, ID to, ID config_id) {
	if (id_is_equal(from, to)) {
		return;
	}

	Constraint_TC * constraint = tc_allocate(ID_TC_CONSTRAINT);
	constraint->from = from;
	constraint->to = to;
	constraint->config_id = config_id;

	solver_link_constraint(constraint);

	DEQUE_PUSH_BACK(ID, &ctx->worklist, constraint->constraint_id);
}

ID config_merge(ID config1_id, ID config2_id, char * is_impossible_flag) {
	if (ID_IS_INVALID(config1_id)) {
		return config2_id;
	} else if (ID_IS_INVALID(config2_id)) {
		return config1_id;
	}

	Configuration_TC * config1_it = lookup(config1_id), * config2_it;
	Configuration_TC * config2 = config2_it = lookup(config2_id);

	Configuration_TC * new_config = tc_allocate(ID_TC_CONFIGURATION);
	Configuration_TC * new_config_it = new_config;
	char conflict = 0;

	// Append all configs in config1 that are not in config2
	do {
		Configuration_TC * config2_it = config2;
		conflict = 0;
		do {
			if (id_is_equal(config1_it->dimension_id, config2_it->dimension_id) && config1_it->dimension_choice != config2_it->dimension_choice) {
				conflict = 1;
			}
		} while (!ID_IS_INVALID(config2_it->next_config) && (config2_it = lookup(config2_it->next_config), 1));

		if (!conflict) {
			new_config_it->dimension_id = config1_it->dimension_id;
			new_config_it->dimension_choice = config1_it->dimension_choice;

			Configuration_TC * temp = tc_allocate(ID_TC_CONFIGURATION);
			new_config_it->next_config = temp->config_id;
			new_config_it = temp;
		} else {
			*is_impossible_flag = 1;
		}
	} while (!ID_IS_INVALID(config1_it->next_config) && (config1_it = lookup(config1_it->next_config), 1));

	// Append all configs in config2
	config2_it = config2;
	do {
		new_config_it->dimension_id = config2_it->dimension_id;
		new_config_it->dimension_choice = config2_it->dimension_choice;

		Configuration_TC * temp = tc_allocate(ID_TC_CONFIGURATION);
		new_config_it->next_config = temp->config_id;
		new_config_it = temp;
	} while (!ID_IS_INVALID(config2_it->next_config) && (config2_it = lookup(config2_it->next_config), 1));

	// If not impossible it will just be the config1 extended with on config2
	return new_config->config_id;
}
