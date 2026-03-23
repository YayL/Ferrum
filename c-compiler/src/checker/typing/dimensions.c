#include "checker/typing/dimensions.h"

#include "tables/registry_manager.h"

DdNode * dimension_get_choice(Solver * solver, ID dimension_id, size_t choice, DdNode * world) {
	DdManager * manager = solver->manager;
	if (manager == NULL) {
		return NULL;
	}

	Dimension_TC * dimension = lookup(dimension_id);
	ASSERT1(choice < (1 << dimension->bit_count));

	DdNode * result = Cudd_ReadOne(manager);
	ASSERT1(result != NULL);
	Cudd_Ref(result);

	// println("{i}({i}): ", choice, 1 << dimension->bit_count);
	for (size_t i = 0; i < dimension->bit_count; ++i) {
		DdNode * var = Cudd_bddIthVar(manager, dimension->first_bit_index + i);
		ASSERT1(var != NULL);

		// println((choice >> i) & 1 ? "\t{i}" : "\t!{i}", i);

		CUDD_AND(tmp, solver, result, (choice >> i) & 1 ? var : Cudd_Not(var));
		Cudd_RecursiveDeref(manager, result);
		result = tmp;
	}

	CUDD_AND(tmp, solver, result, world);
	Cudd_RecursiveDeref(solver->manager, result);
	result = tmp;

	return result;
}

void print_possibilities(Solver * solver, DdNode * state) {
	DdGen * gen = NULL;
	int * cube = NULL;
    char first = 1;

	if (state == Cudd_ReadLogicZero(solver->manager)) {
		println("No possibilities");
		return;
	}

	// We search for primes that cover the 'state' (from state to state)
	gen = Cudd_FirstPrime(solver->manager, state, state, &cube);
    if (!gen) {
        println("(N/A)");
        return;
    }

    do {
        if (!first) {
            print(" OR ");
        }
        print("(");
        char first_dim = 1;

        Dimension_TC * dim;
        LOOP_OVER_REGISTRY(Dimension_TC, dim, {
            // Check if this dimension is "relevant" (at least one bit is not a Don't Care)
            char is_relevant = 0;
            for (int b = 0; b < dim->bit_count; ++b) {
                if (cube[dim->first_bit_index + b] != 2) {
                    is_relevant = 1;
                    break;
                }
            }
            
            if (!is_relevant) continue;
            first = 0;

            if (!first_dim) print(", ");
            print("D{u}[", dim->dimension_id.id);
            first_dim = 0;

            char first_cand = 1;
            for (size_t c = 0; c < (1 << dim->bit_count); ++c) {
                char match = 1;
                for (int b = 0; b < dim->bit_count; ++b) {
                    int bit_val = (c >> b) & 1;
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
            print("]");
        });

        print(")");
    } while (Cudd_NextPrime(gen, &cube));
    println("");
}
