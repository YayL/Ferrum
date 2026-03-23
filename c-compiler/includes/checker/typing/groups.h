#pragma once

#include "common/ID.h"
#include "checker/typing/typechecker.h"
#include "checker/typing/solver.h"

Group_TC * solver_variable_get_root_group(Solver * solver, Variable_TC * variable);
void solver_unify_vars_groups(Solver * solver, ID id1, ID id2, DdNode * world);
void solver_add_group_requirement(struct solver * solver, Group_TC * group, Requirement_TC * requirement);
void solver_add_variable_group_requirement(struct solver * ctx, ID variable_id, ID requirement_type_id, DdNode * world);

void group_print_requirements(Solver * solver, ID group_id);
