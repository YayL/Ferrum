#pragma once

#include "parser/AST.h"
#include "common/ID.h"

void checker_check_if(ID node_id);
void checker_check_while(ID node_id);

ID checker_check_expr_node(ID node_id);
ID checker_check_expression(ID node_id);
ID checker_check_op(ID node_id);
ID checker_check_variable(ID node_id);
ID checker_check_literal(ID node_id);
ID checker_check_symbol(ID node_id);

void checker_check_scope(ID node_id);

void checker_check_declaration(ID node_id);
void checker_check_function(ID node_id);
void checker_check_module(ID node_id);

void checker_check(a_root root);
