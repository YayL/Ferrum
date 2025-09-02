#pragma once

#include "common/ID.h"
#include "common/memory/arena.h"

typedef struct function_resoloution_solver {
	ID name_id;
	ID args_type_id;
	Arena candidates;
} FRSolver;

typedef struct function_resoloution_result {
	ID name_id;
	ID args_type_id;
	ID function_id;
	ID function_return_type_id;
} FRResult;

struct substitution {
	ID symbol_id;	// the symbol ID of the type template
	ID type_id;		// the type ID of the type the template is resolved to
};

FRSolver frsolver_init(ID name_id, ID args_type_id, ID scope_id, Arena candidates);
FRResult frsolver_solve(FRSolver solver);

struct substitution subst_lookup(FRResult fr, ID id);

ID * create_type_variable();
