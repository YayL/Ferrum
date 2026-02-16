#include "checker/typing/dimensions.h"

#include "math.h"
#include "tables/registry_manager.h"

void resolver_encode_choice(Dim_Resolver resolver, ID dimension_id, size_t choice) {
	ASSERT1(!ID_IS_INVALID(dimension_id));
	ASSERT1(choice > 0);


}

void dimension_init_choices(Dim_Resolver resolver, Dimension_TC * dimension) { 
	ASSERT1(dimension->candidates.size > 0);

	const size_t bits = floor(log2(dimension->candidates.size + 1));
	dimension->bit_variables = arena_init(sizeof(DdNode *));
	arena_grow(&dimension->bit_variables, bits);

	for (size_t i = 0; i < bits; ++i) {
		DdNode * var = Cudd_bddNewVar(resolver.manager);
		Cudd_Ref(var);
		ARENA_APPEND(&dimension->bit_variables, var);
	}
}

DdNode * dimension_get_choice(Dim_Resolver resolver, ID dimension_id, size_t choice) {
	Dimension_TC * dimension = lookup(dimension_id);
	ASSERT1(choice < dimension->candidates.size);

	DdNode * result = Cudd_ReadOne(resolver.manager);
	Cudd_Ref(result);

	for (size_t i = 0; i < dimension->bit_variables.size; ++i) {
		DdNode * var = ARENA_GET(dimension->bit_variables, i, DdNode *), * next_result;

		next_result = Cudd_bddAnd(resolver.manager, result, (choice >> i) & 1 ? var : Cudd_Not(var));
		Cudd_Ref(next_result);
		Cudd_RecursiveDeref(resolver.manager, result);
		result = next_result;
	}

	return result;
}
