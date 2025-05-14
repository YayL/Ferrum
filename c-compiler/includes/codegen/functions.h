#pragma once

#include "common/common.h"
#include "parser/operators.h"

struct AST * get_member_function(const char * member_name, Type arguments_type, Type return_type, struct AST * scope);
struct AST * get_function_for_operator(struct Operator * op, Type left, Type right, Type * self_type, struct AST * scope);
struct AST * get_declared_function(const char * name, struct Arena list1, struct AST * scope);

char is_declared_function(char * name, struct AST * scope);
