#include "codegen/functions.h"

#include "checker/typing.h"
#include "codegen/AST.h"
#include "codegen/checker.h"
#include "common/arena.h"
#include "common/list.h"
#include "common/logger.h"
#include "common/macro.h"
#include "fmt.h"
#include "parser/operators.h"
#include "parser/types.h"
#include <stdlib.h>

void resolve_function_from_call(struct AST * ast) {
    a_op * op = &ast->value.operator;
    struct AST * left = op->left, * right = op->right;
    ASSERT1(left->type == AST_VARIABLE);

    const char * function_name = left->value.variable.name;
    Type args_type = ast_to_type(right);

    FRResult result = frsolver_solve(frsolver_init(function_name, args_type, ast->scope));

    if (result.function == NULL) {
        ERROR("Function {s}({s}) is not defined", op->op->str, type_to_str(args_type));
        exit(1);
    }

    op->definition.function = result.function;
    op->definition.substitution = result.substitutions;
}

void resolve_function_from_operator(struct AST * ast) {
    a_op * op = &ast->value.operator;
    struct AST * left = op->left, * right = op->right;

    Type * type = op->type = calloc(1, sizeof(Type));
    type->value = init_intrinsic_type(ITuple);
    type->intrinsic = ITuple;

    Type * lhs_type = ast_get_type_of(left), * rhs_type = ast_get_type_of(right);
    if (op->op->mode == BINARY) {
        ASSERT1(lhs_type != NULL);
    }
    ASSERT1(rhs_type != NULL);

    Arena * arena = &type->value.tuple.types;
    if (op->op->mode == BINARY) {
        ARENA_APPEND(arena, *lhs_type);
    }
    ARENA_APPEND(arena, *rhs_type);

    const char * function_name = get_operator_runtime_name(op->op->key);
    FRResult result = frsolver_solve(frsolver_init(function_name, *type, ast->scope));

    if (result.function == NULL) {
        ERROR("Operator '{s}'({s}) is not defined for {s}", op->op->str, function_name, type_to_str(*type));
        exit(1);
    }

    op->definition.function = result.function;
    op->definition.substitution = result.substitutions;
}

struct AST * get_member_function(const char * member_name, Type args_type, Type return_type, struct AST * scope) {
    ASSERT1(args_type.intrinsic != IUnknown);
    ASSERT1(return_type.intrinsic != IUnknown);

    // a_module module = get_scope(AST_MODULE, scope)->value.module;
    // // struct List * functions = hashmap_get(module.functions_map, member_name);
    // Arena argument_types = type_to_type_arena(args_type);
    //
    // if (functions == NULL || functions->size == 0) {
    //     FATAL("Unable to find member function: '{s}'", member_name);
    // }
    //
    // println("Search: {s}, Results: {i}", member_name, functions->size);
    //
    // for (int i = 0; i < functions->size; ++i) {
    //     print_ast("Func: {s}\n", list_at(functions, i));
    // }
    //
    // struct AST * func;
    // for (int i = 0; i < functions->size; ++i) {
    //     func = list_at(functions, i);
    //
    //     ASSERT(func->type == AST_FUNCTION, "Member must be a function!");
    //
    //     a_function function = func->value.function;
    //     Arena function_argument_types = type_to_type_arena(*function.param_type);
    //    
    //     if (function_argument_types.size != argument_types.size) {
    //         continue;
    //     }
    //    
    //     DEBUG("fn scope: {s}", ast_type_to_str(func->scope->type));
    //     if (function.parsed_templates->size != 0) {
    //         char * type_name = list_at(function.parsed_templates, 0);
    //         // print_ast(format("Type ({s!}): {s}\n", type_name), hashmap_get(function.template_types, type_name));
    //     }
    //
    //     char found = 1;
    //     // for (int j = 0; j < function_argument_types->size || !(found); ++j) {
    //     //     Type item1 = DEREF_AST(list_at(arguments_type, j)).type,
    //     //          item2 = DEREF_AST(list_at(function_argument_types, j)).type;
    //     //     if (!check_types(item1, item2, function.template_types)) {
    //     //         found = 0;
    //     //         break;
    //     //     }
    //     // }
    //
    //     if (found) {
    //         return func;
    //     }
    // }

    return NULL;
}

struct AST * get_function_for_operator(struct Operator * op, Type lhs, Type rhs, Type * self_type, struct AST * scope) {
    Arena arg_types_arena = arena_init(sizeof(Type));

    DEBUG("LHS: {s}", type_to_str(lhs));
    DEBUG("RHS: {s}", type_to_str(lhs));

    if (lhs.intrinsic == IUnknown) {
        ARENA_APPEND(&arg_types_arena, lhs);
    }
    ARENA_APPEND(&arg_types_arena, rhs);

    ASSERT1(self_type != NULL);
    if (self_type->intrinsic == IUnknown) {
        *self_type = *((Type *) arena_get(arg_types_arena, 0));
    }

    const char * name = get_operator_runtime_name(op->key);
    struct AST * func = get_member_function(name, rhs, lhs, scope);

    DEBUG("Name: {s}", name);
    DEBUG("Scope: {s}", ast_to_string(scope));
    DEBUG("Self: {s}", type_to_str(*self_type));

    if (func == NULL) {
        ASSERT1(rhs.intrinsic != IUnknown);
 
        if (lhs.intrinsic != IUnknown) {
            DEBUG("Left: {s}", type_to_str(lhs));
        }

        DEBUG("Right: {s}", type_to_str(rhs));

        if (lhs.intrinsic != IUnknown) {
            ERROR("Operator '{s}'({s}) is not defined for ({2s:, })", op->str, name, type_to_str(lhs), type_to_str(rhs));
        } else {
            ERROR("Operator '{s}'({s}) is not defined for ({s})", op->str, name, type_to_str(rhs));
        }

        exit(1);
    }

    return func;
}

struct AST * get_declared_function(const char * name, Arena list1, struct AST * scope) {
    scope = get_scope(AST_MODULE, scope); 
    a_module module = scope->value.module;

    for (int i = 0; i < module.functions->size; ++i) {
        struct AST * node = list_at(module.functions, i);
        a_function function = node->value.function;

        if (strcmp(function.name, name))
            continue;

        Arena list2 = type_to_type_arena(*function.param_type);

        if (list1.size != list2.size)
            continue;

        char found = 1;
        for (int j = 0; j < list1.size; ++j) {
            Type * item1 = arena_get(list1, j),
                 * item2 = arena_get(list2, j);
            if (!is_equal_type(*item1, *item2)) {
                print_ast("{s}\n", node);
                found = 0;
                break;
            }
        }
        if (found)
            return node;
    }

    return NULL;
}

char is_declared_function(char * name, struct AST * scope) {
    scope = get_scope(AST_MODULE, scope);

    a_module module = scope->value.module;
    for (int i = 0; i < module.functions->size; ++i) {
        a_function function = DEREF_AST(list_at(module.functions, i)).function;
        if (!strcmp(function.name, name))
            return 1;
    }

    return 0;
}
