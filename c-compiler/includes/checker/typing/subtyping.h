#pragma once

#include "common/ID.h"

typedef struct solver Solver;

char is_subtype(Solver * solver, ID A, ID B);
char is_subtype_strict(Solver * solver, ID A, ID B);
char is_implicit_subtype(Solver * solver, ID lower, ID upper);
char instantiate_solve(Solver * solver, ID A, ID B);

char subtype_check_equal(Solver * solver, ID type_id1, ID type_id2);
