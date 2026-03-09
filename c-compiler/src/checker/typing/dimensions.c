#include "checker/typing/dimensions.h"

#include "tables/registry_manager.h"

Dim_Resolver dimension_resolver_init() {
	DdManager * manager = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
	Dim_Resolver resolver = { .manager = manager, .bit_variable_count = 0, .state = NULL };

	// Dimension_TC * dimension;

	// LOOP_OVER_REGISTRY(Dimension_TC, dimension, {
	// 		dimension_init_choices(&resolver, dimension, NULL);
	// });

	// resolver.state = state;
	// resolver_print_possibilities(resolver);
	//
	// DdNode * choice_1 = dimension_get_choice(resolver, (ID) {.id = 3, .type = ID_TC_DIMENSION}, 0);
	// DdNode * choice_2 = dimension_get_choice(resolver, (ID) {.id = 3, .type = ID_TC_DIMENSION}, 2);
	// DdNode * choice_3 = dimension_get_choice(resolver, (ID) {.id = 1, .type = ID_TC_DIMENSION}, 2);
	//
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_1), state);
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_2), state);
	// state = Cudd_bddAnd(resolver.manager, Cudd_Not(choice_3), state);
	//
	// resolver.state = state;
	// resolver_print_possibilities(resolver);

	return resolver;
}

DdNode * dimension_get_choice(Dim_Resolver resolver, ID dimension_id, size_t choice) {
	if (resolver.manager == NULL) {
		return NULL;
	}

	Dimension_TC * dimension = lookup(dimension_id);
	ASSERT1(choice < (1 << dimension->bit_count));

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

	ASSERT1(resolver->state != NULL);
	DdNode * new_state = Cudd_bddAnd(resolver->manager, resolver->state, Cudd_Not(choice));
	ASSERT1(new_state != NULL);
	Cudd_Ref(new_state);
	Cudd_RecursiveDeref(resolver->manager, resolver->state);
	resolver->state = new_state;
}

void print_possibilities(Dim_Resolver resolver, DdNode * state) {
	DdGen * gen = NULL;
	int * cube = NULL;

	// We search for primes that cover the 'state' (from state to state)
	gen = Cudd_FirstPrime(resolver.manager, state, state, &cube);
    if (!gen) return;

	// Iterate over every path that leads to the 'One' terminal
	do {
		print("(");
		char first_dim = 1;

		Dimension_TC * dim;
		LOOP_OVER_REGISTRY(Dimension_TC, dim, {
			char is_relevant = 0;
			for (int b = 0; b < dim->bit_count; ++b) {
				if (cube[dim->first_bit_index + b] != 2) {
					is_relevant = 1;
					break;
				}
			}

			if (!is_relevant) {
				continue;
			}

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

				if (!match) {
					continue;
				}

				if (!first_cand) {
					print("|");
				}

				print("{u}", c);
				first_cand = 0;
			}
		});

		println("])");
	} while (Cudd_NextPrime(gen, &cube));
}
