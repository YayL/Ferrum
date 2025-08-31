#include "checker/typing.h"

#include "parser/AST.h"
#include "tables/registry_manager.h"

FRSolver frsolver_init(ID name_id, ID args_type_id, ID scope_id) {
	a_module * module = get_scope(ID_AST_MODULE, scope_id);
	ASSERT1(module != NULL);

	// struct List * candidates = hashmap_get(module->value.module.functions_map, name);
	// ASSERT1(candidates != NULL);
	return (FRSolver) { .name_id = name_id, args_type_id = args_type_id, /* candidates = candidates */ };
}

FRResult frresult_init(FRSolver solver) {
	return (FRResult) { .name_id = solver.name_id, .args_type_id = solver.args_type_id, .function_id = INVALID_ID };
}

char frsolver_check_candidate(FRSolver solver, ID candidate_id, struct arena * substitutions) {
	ASSERT1(ID_IS(solver.args_type_id, ID_TUPLE_TYPE));

	a_function candidate = LOOKUP(candidate_id, a_function);
	ASSERT1(ID_IS(candidate.param_type, ID_TUPLE_TYPE));

	Arena call_args_type = LOOKUP(solver.args_type_id, Tuple_T).types;
	Arena func_args_type = LOOKUP(candidate.param_type, Tuple_T).types;

	// Must have same amount of arguments
	if (call_args_type.size != func_args_type.size) {
		return 0;
	}

	arena_clear(substitutions);

	for (size_t i = 0; i < call_args_type.size; ++i) {
		ID call_arg_type = ARENA_GET(call_args_type, i, ID);
		ID func_arg_type = ARENA_GET(func_args_type, i, ID);
		ASSERT1(!ID_IS_INVALID(call_arg_type));
		ASSERT1(!ID_IS_INVALID(func_arg_type));

		// if (func_arg_type.intrinsic != IVariable && is_equal_type(, Type type2, struct HashMap *self)) {
		//
		// }
	}
	// println("Candidate: {s}", ast_to_string(candidate));

	return 0;
}

FRResult frsolver_solve(FRSolver solver) {
	FRResult result = frresult_init(solver);
	struct arena substitutions = arena_init(sizeof(struct substitution));

	for (size_t i = 0; i < solver.candidates.size; ++i) {
		ID candidate_id = ARENA_GET(solver.candidates, i, ID);
		ASSERT1(!ID_IS_INVALID(candidate_id));

		if (frsolver_check_candidate(solver, candidate_id, &substitutions)) {
			if (!ID_IS_INVALID(result.function_id)) {
				ERROR("There are multiple function candidates, unable to resolve");
				exit(1);
			}

			result.substitutions = substitutions;
			result.function_id = candidate_id;
			break;
		}
	}

	return result;
}

// Type * create_type_variable() {
// 	Variable_T * var = malloc(sizeof(Variable_T));
// 	var->ID = next_var_id++;
// 	var->type = NULL;
//
// 	// Type * type = malloc(size_t size)
// }
