#pragma once

#include "common/ID.h"

typedef struct solver Solver;

char is_subtype(Solver * solver, ID A, ID B, char is_strict);
char is_implicit_subtype(Solver * solver, ID lower, ID upper);
char is_subtype_strict(Solver * solver, ID A, ID B);
char instantiate_solve(Solver * solver, ID A, ID B, char is_strict);

char subtype_check_equal(Solver * solver, ID type_id1, ID type_id2, char is_strict);
