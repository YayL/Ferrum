#pragma once

#include "parser/types.h"

typedef struct function_resoloution_solver {
	unsigned int name_id;
	Type args;
	struct List * candidates;
} FRSolver;

typedef struct function_resoloution_result {
	unsigned int name_id;
	Type args;
	struct AST * function;
	Arena substitutions;
} FRResult;

struct substitution {
	unsigned int variable_id; // this should be the same as the index
	Type substitution_type;
};

FRSolver frsolver_init(unsigned int name_id, Type args, struct AST * scope);
FRResult frsolver_solve(FRSolver solver);

struct substitution subst_lookup(FRResult fr, unsigned int ID);

Type * create_type_variable();
