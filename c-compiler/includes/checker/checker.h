#pragma once

#include "parser/AST.h"
#include "common/ID.h"

typedef struct solver Solver;

void checker_check_scope(Solver * solver, ID node_id);
void checker_check_declaration(Solver * solver, ID node_id);
void checker_check_function(Solver * solver, ID node_id);
void checker_check_module(Solver * solver, ID node_id);

void checker_check(a_root root);

#include "checker/typing/solver.h"
