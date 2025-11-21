#include "checker/functions.h"

#include "checker/typing.h"
#include "parser/AST.h"
#include "tables/registry_manager.h"

#include "checker/context.h"
#include "tables/member_functions.h"

FRSolver frsolver_init(ID name_id, ID args_type_id, ID scope_id, Arena candidates) {
	a_module * module = get_scope(ID_AST_MODULE, scope_id);
	ASSERT1(module != NULL);

	return (FRSolver) { .name_id = name_id, args_type_id = args_type_id, .candidates = candidates };
}

FRResult frresult_init(FRSolver solver) {
	return (FRResult) { .name_id = solver.name_id, .args_type_id = solver.args_type_id, .function_id = INVALID_ID };
}

char frsolver_check_candidate(FRSolver solver, a_function candidate, khash_t(map_id_to_id) * templates, unsigned int * specificity_cost) {
	ASSERT1(ID_IS(solver.args_type_id, ID_TUPLE_TYPE));
	ASSERT1(ID_IS(candidate.type, ID_FN_TYPE));

	Arena call_arg_types = LOOKUP(solver.args_type_id, Tuple_T).types;

	Fn_T fn = LOOKUP(candidate.type, Fn_T);
	ASSERT1(ID_IS(fn.arg_type, ID_TUPLE_TYPE));
	Arena func_arg_types = LOOKUP(fn.arg_type, Tuple_T).types;

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

		if (!is_valid_equal_type(call_arg_type, func_arg_type, NULL, templates, specificity_cost)) {
			return 0;
		}
	}

	return 1;
}

FRResult frsolver_solve(FRSolver solver) {
	FRResult result = frresult_init(solver);
	khash_t(map_id_to_id) templates = kh_init(map_id_to_id);
	unsigned int best_specificity_cost = UINT32_MAX;

	for (size_t i = 0; i < solver.candidates.size; ++i) {
		ID candidate_id = ARENA_GET(solver.candidates, i, ID);
		ASSERT1(ID_IS(candidate_id, ID_AST_FUNCTION));

		a_function candidate = LOOKUP(candidate_id, a_function);
		kh_clear(map_id_to_id, &templates);

		switch (candidate.info.scope_id.type) {
			case ID_AST_IMPL: {
				a_implementation impl = LOOKUP(candidate.info.scope_id, a_implementation);
				populate_template_hashmap_with_impl(impl, &templates);
			} break;
			case ID_AST_STRUCT: {
				a_structure _struct = LOOKUP(candidate.info.scope_id, a_structure);
				populate_template_hashmap_by_arena(_struct.templates, &templates, 1);
			} break;
			default: break;
		}

		unsigned int specificity_cost = 0;

		if (frsolver_check_candidate(solver, candidate, &templates, &specificity_cost)) {
			char is_match = 1;

			ID key, value;
			kh_foreach(&templates, key, value, {
				ASSERT1(!ID_IS_INVALID(key));
				if (ID_IS_INVALID(value)) {
					ERROR("Unable to determine type of the template type '{s}'", interner_lookup_str(key)._ptr);
					is_match = 0;
					break;
				}

				if (ID_IS(value, ID_AST_VARIABLE)) {
					a_variable var = LOOKUP(value, a_variable);
					value = var.type_id;
				}
			});

			if (!is_match) {
				continue; // Check next candidate
			}

			if (!check_templates_uphold_template_rules(templates, candidate.templates)) {
				continue; // Check next candidate
			}

			is_match = 1;
			switch (candidate.info.scope_id.type) {
				case ID_AST_IMPL: {
					a_implementation impl = LOOKUP(candidate.info.scope_id, a_implementation);
					if  (!check_templates_uphold_template_rules(templates, impl.generic_templates)) {
						is_match = 0;
						break;
					}
				} break;
				default: break;
			}

			if (!is_match || best_specificity_cost < specificity_cost) {
				continue;
			}


			Fn_T fn = LOOKUP(candidate.type, Fn_T);
			if (specificity_cost == best_specificity_cost) {
				ASSERT1(!ID_IS_INVALID(result.function_id));
				ERROR("There are multiple function candidates, unable to resolve");
				ERROR("First(cost={u}): {s}", best_specificity_cost, type_to_str(result.function_return_type_id));
				print_ast_tree(result.function_id);
				ERROR("Second(cost={u}): {s}", specificity_cost, type_to_str(resolve_type_templates_in_type(fn.ret_type, &templates)));
				print_ast_tree(candidate_id);
				exit(1);
			}

			best_specificity_cost = specificity_cost;
			result.function_return_type_id = resolve_type_templates_in_type(fn.ret_type, &templates);
			result.function_id = candidate_id;
			/* TODO: Templates have been resolved so keep that information (Required for codegen) */
		}
	}

	kh_free(map_id_to_id, &templates);

	return result;
}

void resolve_function_from_call(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * call_op = lookup(node_id);

	Arena candidates = {0};
	ID name_id = INVALID_ID, scope_id = INVALID_ID;

	switch (call_op->left_id.type) {
		case ID_AST_SYMBOL: {
			a_symbol function_symbol = LOOKUP(call_op->left_id, a_symbol);
			name_id = function_symbol.name_id;
			scope_id = function_symbol.info.scope_id;
			candidates = context_lookup_all_declarations(name_id);
		} break;
		case ID_AST_OP: {
			a_operator op = LOOKUP(call_op->left_id, a_operator);

			if (op.op.key != MEMBER_ACCESS) {
				FATAL("Invalid function call LHS operator");
			}

			ID type_id = ast_get_type_of(op.left_id);
			a_symbol member_access_rhs = LOOKUP(op.right_id, a_symbol);

			ASSERT1(ID_IS(call_op->right_id, ID_AST_EXPR));
			a_expression * expr = lookup(call_op->right_id);

			arena_next(&expr->children);
			for (ssize_t i = expr->children.size - 1; i > 0; --i) {
				ARENA_GET(expr->children, i, ID) = ARENA_GET(expr->children, i - 1, ID);
			}
			ARENA_GET(expr->children, 0, ID) = op.left_id;

			println("type: {s}", type_to_str(type_id));

			print_ast_tree(node_id);

			ASSERT1(member_access_rhs.name_ids.size == 1);
			name_id = member_access_rhs.name_id;
			scope_id = op.info.scope_id;
			candidates = member_function_index_lookup(name_id);
		} break;
		default:
			FATAL("Invalid function call LHS");
	}

    ID args_type_id = ast_to_type(call_op->right_id);

	// println("Call '{s}', Type: {s}, Scope: {s}, Candidates: {i}", interner_lookup_str(name_id)._ptr, type_to_str(args_type_id), id_type_to_string(scope_id.type), candidates.size);

    FRResult result = frsolver_solve(frsolver_init(name_id, args_type_id, scope_id, candidates));

    if (ID_IS_INVALID(result.function_id)) {
        ERROR("Function {s}{s} is not defined", interner_lookup_str(name_id)._ptr, type_to_str(args_type_id));
        exit(1);
    }

    call_op->definition.function_id = result.function_id;
    call_op->type_id = result.function_return_type_id;
}

void resolve_function_from_operator(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * op = lookup(node_id);

    ID left_id = op->left_id, right_id = op->right_id;

    Arena tuple_types = arena_init(sizeof(ID));

    if (op->op.mode == BINARY) {
        ARENA_APPEND(&tuple_types, ast_get_type_of(left_id));
    }
    ARENA_APPEND(&tuple_types, ast_get_type_of(right_id));

    ID name_id = operator_get_intern_id(op->op.key);

    ID args_type_id = type_from_arena(tuple_types);
    Arena candidates = member_function_index_lookup(name_id);

    FRResult result = frsolver_solve(frsolver_init(name_id, args_type_id, op->info.scope_id, candidates));

    if (ID_IS_INVALID(result.function_id)) {
        ERROR("Operator '{s}'{s} is not defined for {s}", op->op.str, operator_get_runtime_name(op->op.key), type_to_str(args_type_id));
        exit(1);
    }

    op->definition.function_id = result.function_id;
    op->type_id = result.function_return_type_id;

    // println("type: {s}, {s}", type_to_str(op->type_id), id_type_to_string(op->type_id.type));
    // println("args: {s}", type_to_str(result.args_type_id));
    // print_ast_tree(result.function_id);
    // print_ast_tree(node_id);

    if (ID_IS(op->type_id, ID_ARRAY_TYPE)) {
        exit(0);
    }
}
