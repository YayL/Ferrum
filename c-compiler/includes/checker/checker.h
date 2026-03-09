#pragma once

#include "parser/AST.h"
#include "common/ID.h"

typedef struct solver Solver;

char check_has_member(ID node_id, ID member_name_id);

void checker_check_if(Solver * solver, ID node_id, const Arena templates);
void checker_check_while(Solver * solver, ID node_id, const Arena templates);

ID checker_check_expr_node(Solver * solver, ID node_id, const Arena templates);
ID checker_check_expression(Solver * solver, ID node_id, const Arena templates);
ID checker_check_op(Solver * solver, ID node_id, const Arena templates);
ID checker_check_variable(Solver * solver, ID node_id, const Arena templates);
ID checker_check_symbol(Solver * solver, ID node_id, const Arena templates);
ID checker_check_literal(Solver * solver, ID node_id);

void checker_check_scope(Solver * solver, ID node_id, const Arena templates);

void checker_check_declaration(Solver * solver, ID node_id, const Arena templates);
void checker_check_function(Solver * solver, ID node_id, Arena * templates_ref);
void checker_check_module(Solver * solver, ID node_id);

void checker_check(a_root root);

#include "checker/typing/solver.h"
