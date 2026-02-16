#include "checker/typing/dimensions.h"

#include "tables/registry_manager.h"

DdNode * dimension_get_choice(Dim_Resolver resolver, ID dimension_id, size_t choice) {
	Dimension_TC * dimension = lookup(dimension_id);
	ASSERT1(choice < dimension->candidates.size);

	DdNode * result = Cudd_ReadOne(resolver.manager);
	ASSERT1(result != NULL);
	Cudd_Ref(result);

	// println("{i}({i}): ", choice, dimension->bit_variables.size);
	for (size_t i = 0; i < dimension->bit_count; ++i) {
		DdNode * var = Cudd_bddIthVar(resolver.manager, dimension->first_bit_index + i);
		ASSERT1(var != NULL);

		// println((choice >> i) & 1 ? "\t{i}" : "\t!{i}", i);

		DdNode * next_result = Cudd_bddAnd(resolver.manager, result, (choice >> i) & 1 ? var : Cudd_Not(var));
		ASSERT1(next_result != NULL);
		Cudd_Ref(next_result);
		Cudd_RecursiveDeref(resolver.manager, result);
		result = next_result;
	}

	return result;
}

void resolver_add_invalid_choice(Dim_Resolver * resolver, DdNode * choice) {
	if (choice == NULL) {
		return;
	}

	DdNode * new_state = Cudd_bddAnd(resolver->manager, resolver->state, Cudd_Not(choice));
	Cudd_Ref(new_state);
	Cudd_RecursiveDeref(resolver->manager, resolver->state);
	resolver->state = new_state;
}

void resolver_print_possibilities(Dim_Resolver resolver) {
	DdGen *gen;
	int *cube;
	CUDD_VALUE_TYPE value;

	// Iterate over every path that leads to the 'One' terminal
	Cudd_ForeachCube(resolver.manager, resolver.state, gen, cube, value) {
		print("(");
		char first_dim = 1;

		Dimension_TC *dim;
		LOOP_OVER_REGISTRY(Dimension_TC, dim, {
			if (!first_dim) print("], ");
			first_dim = 0;

			print("D{u}[", dim->dimension_id.id);

			char first_cand = 1;
			for (size_t c = 0; c < dim->candidates.size; ++c) {
				char match = 1;
				for (int b = 0; b < dim->bit_count; ++b) {
					int bit_val = (c >> b) & 1;
					// cube[i] is 0, 1, or 2 (don't care)
					int cube_val = cube[dim->first_bit_index + b];
					if (cube_val != 2 && cube_val != bit_val) {
						match = 0;
						break;
					}
				}

				if (match) {
					if (!first_cand) print("|");
					print("{u}", c);
					first_cand = 0;
				}
			}
		});

		println("])");
	}}
