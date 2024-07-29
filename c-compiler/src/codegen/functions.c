#include "codegen/functions.h"

#include "codegen/AST.h"
#include "codegen/checker.h"
#include "common/list.h"
#include "parser/types.h"

struct Ast * get_member_function(const char * member_name, struct Ast * marker, struct List * arguments_type, struct Ast * scope) {
    if (marker == NULL) {
        logger_log(format("Unknown type for marker when calling '{s}'", member_name), CHECKER, ERROR);
        exit(1);
    }

    a_module * module = get_scope(AST_MODULE, scope)->value;
    struct List * functions = hashmap_get(module->functions_map, member_name);

    if (functions->size == 0) {
        logger_log(format("Unable to find member function: '{s}'", member_name), CHECKER, ERROR);
        exit(1);
    }

    println("Search: {s}, Results: {i}", member_name, functions->size);
    
    for (int i = 0; i < functions->size; ++i) {
        print_ast("Func: {s}\n", list_at(functions, i));
    }

    struct Ast * func;
    for (int i = 0; i < functions->size; ++i) {
        func = list_at(functions, i);

        ASSERT(func->type == AST_FUNCTION, "Member must be a function!");

        a_function * function = func->value;
        struct List * function_argument_types = ast_to_ast_type_list(function->param_type);
        
        if (function_argument_types->size != arguments_type->size) {
            continue;
        }
        
        println("fn scope: {s}", ast_type_to_str(func->scope->type));
        if (function->parsed_templates->size != 0) {
            char * type_name = list_at(function->parsed_templates, 0);
            print_ast(format("Type ({s!}): {s}\n", type_name), hashmap_get(function->template_types, type_name));
        }

        char found = 1;
        for (int j = 0; j < function_argument_types->size || !(found); ++j) {
            Type * item1 = DEREF_AST(list_at(arguments_type, j)),
                 * item2 = DEREF_AST(list_at(function_argument_types, j));
            if (!check_types(item1, item2, function->template_types)) {
                found = 0;
                break;
            }
        }

        if (found) {
            return func;
        }
    }

    return NULL;
}

struct Ast * get_function_for_operator(struct Operator * op, struct Ast * left, struct Ast * right, struct Ast ** self_type, struct Ast * scope) {
    struct List * arg_types_list = init_list(sizeof(struct Ast *));

    if (left != NULL) {
        list_push(arg_types_list, left);
    }
    list_push(arg_types_list, right);

    if (*self_type == NULL) {
        struct Ast * ast = list_at(arg_types_list, 0);
        Type * type = get_base_type(ast->value);
        ast = init_ast(AST_TYPE, ast->scope);
        ast->value = type;
        *self_type = ast;
    }

    const char * name = get_operator_runtime_name(op->key);
    struct Ast * func = get_member_function(name, *self_type, arg_types_list, scope);

    println("Name: {s}", name);
    print_ast("Scope: {s}\n", scope);
    print_ast("self: {s}\n", *self_type);

    if (func == NULL) {
        ASSERT1(right != NULL);
        ASSERT1(right->value != NULL);
 
        if (left != NULL) {
            print_ast("left: {s}\n", left);
        }
        print_ast("right: {s}\n", right);

        if (left != NULL)
            logger_log(format("Operator '{s}'({s}) is not defined for ({2s:, })", op->str, name, type_to_str(left->value), type_to_str(right->value)), CHECKER, ERROR);
        else 
            logger_log(format("Operator '{s}'({s}) is not defined for ({s})", op->str, name, type_to_str(right->value)), CHECKER, ERROR);

        ASSERT1(0);
    }

    return func;
}

struct Ast * get_declared_function(const char * name, struct List * list1, struct Ast * scope) {
    scope = get_scope(AST_MODULE, scope); 
    a_module * module = scope->value;

    for (int i = 0; i < module->functions->size; ++i) {
        struct Ast * node = list_at(module->functions, i);
        a_function * function = node->value;

        if (strcmp(function->name, name))
            continue;

        struct List * list2 = ast_to_ast_type_list(function->param_type);

        if (list1->size != list2->size)
            continue;

        char found = 1;
        for (int j = 0; j < list1->size; ++j) {
            void * item1 = DEREF_AST(list_at(list1, j)),
                 * item2 = DEREF_AST(list_at(list2, j));
            if (!is_equal_type(item1, item2, NULL)) {
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

char is_declared_function(char * name, struct Ast * scope) {
    scope = get_scope(AST_MODULE, scope);

    a_module * module = scope->value;
    for (int i = 0; i < module->functions->size; ++i) {
        a_function * function = ((struct Ast *)list_at(module->functions, i))->value;
        if (!strcmp(function->name, name))
            return 1;
    }

    return 0;
}
