#pragma once

#include "common/ID.h"
#include "checker/typing/typechecker.h"
#include "checker/typing/solver.h"

Group_TC * group_find_root_group(ID group_id);
void solver_add_group_requirement(struct solver * solver, Group_TC * group, Requirement_TC * requirement, DdNode * world);
void solver_add_variable_group_requirement(struct solver * ctx, ID variable_id, ID requirement_type_id, DdNode * world);
