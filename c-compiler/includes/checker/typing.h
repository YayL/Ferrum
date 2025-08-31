#pragma once

#include "common/ID.h"
#include "common/arena.h"

typedef struct function_resoloution_solver {
	ID name_id;
	ID args_type_id;
	Arena candidates;
} FRSolver;

typedef struct function_resoloution_result {
	ID name_id;
	ID args_type_id;
	ID function_id;
	Arena substitutions;
} FRResult;

struct substitution {
	ID variable_id; // this should be the same as the index
	ID substitution_type_id;
};

FRSolver frsolver_init(ID name_id, ID args, ID scope_id);
FRResult frsolver_solve(FRSolver solver);

struct substitution subst_lookup(FRResult fr, ID id);

ID * create_type_variable();
