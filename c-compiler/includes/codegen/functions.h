#pragma once

#include "common/common.h"
#include "parser/operators.h"

void resolve_function_from_call(struct AST * ast);
void resolve_function_from_operator(struct AST * ast);

struct AST * get_member_function(const char * member_name, Type arguments_type, Type return_type, struct AST * scope);
struct AST * get_function_for_operator(struct Operator * op, Type left, Type right, Type * self_type, struct AST * scope);
struct AST * get_declared_function(unsigned int name_id, Arena list1, struct AST * scope);

char is_declared_function(unsigned int name_id, struct AST * scope);
