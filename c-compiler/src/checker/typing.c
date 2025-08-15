#include "checker/typing.h"
#include "codegen/AST.h"
#include "common/arena.h"
#include "common/hashmap.h"
#include "common/list.h"
#include "common/logger.h"
#include "fmt.h"
#include "parser/types.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

FRSolver frsolver_init(const char * name, Type args, struct AST * scope) {
	struct AST * module = get_scope(AST_MODULE, scope);
	ASSERT1(module != NULL);
	ASSERT1(module->type == AST_MODULE);

	// struct List * candidates = hashmap_get(module->value.module.functions_map, name);
	// ASSERT1(candidates != NULL);
	return (FRSolver) { .name = name, args = args, /* candidates = candidates */ };
}

FRResult frresult_init(FRSolver solver) {
	return (FRResult) { .name = solver.name, .args = solver.args, .function = NULL };
}

char frsolver_check_candidate(FRSolver solver, struct AST * candidate, struct arena * substitutions) {
	ASSERT1(solver.args.intrinsic == ITuple);
	ASSERT1(candidate->value.function.param_type->intrinsic == ITuple);
	struct arena call_args_type = solver.args.value.tuple.types;
	struct arena func_args_type = candidate->value.function.param_type->value.tuple.types;

	// Must have same amount of arguments
	if (call_args_type.size != func_args_type.size) {
		return 0;
	}

	arena_clear(substitutions);

	for (size_t i = 0; i < call_args_type.size; ++i) {
		Type call_arg_type = *(Type *) arena_get(call_args_type, i);
		Type func_arg_type = *(Type *) arena_get(func_args_type, i);
		ASSERT1(func_arg_type.intrinsic != IUnknown);

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

	for (size_t i = 0; i < solver.candidates->size; ++i) {
		struct AST * candidate = list_at(solver.candidates, i);
		ASSERT1(candidate != NULL);

		if (frsolver_check_candidate(solver, candidate, &substitutions)) {
			if (result.function != NULL) {
				ERROR("There are multiple function candidates, unable to resolve");
				exit(1);
			}
			result.substitutions = substitutions;
			result.function = candidate;
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
