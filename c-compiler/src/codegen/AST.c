#include "codegen/AST.h"
#include "common/logger.h"
#include "common/string.h"
#include "fmt.h"
#include "parser/types.h"
#include "tables/interner.h"
#include "tables/symbol.h"
#include <stdlib.h>

#define PADDING_DIRECT_CHILD  ""
#define PADDING_LIST_CHILDREN "│"
#define AST_TREE_PADDING(comp) (comp ? PADDING_LIST_CHILDREN : PADDING_DIRECT_CHILD)
#define AST_TREE_PRINT_CHILDREN(list, pstring) {for (int i = 0; i < list->size; ++i){ _print_ast_tree(list_at(list, i), pstring, i == (list->size - 1));}}
#define AST_TREE_PRINT_CHILDREN_REVERSE(list, pstring) {for (ssize_t i = list->size - 1; 0 <= i; --i){ _print_ast_tree(list_at(list, i), pstring, i == 0);}}

struct AST * init_ast(enum AST_type type, struct AST * scope) {
    struct AST * ast = malloc(sizeof(struct AST));
    ast->type = type;
    ast->scope = scope;
    ast->value = init_ast_value(type);

    return ast;
}

union AST_union init_ast_value(enum AST_type type) {
    union AST_union value = {0};
    switch (type) {
        case AST_ROOT:
            {
                value.root.modules = init_list(sizeof(a_module *));
                // value.root.markers = hashmap_init(8);
            } break;
        case AST_MODULE:
            {
                value.module.symbol_table = symbol_table_init();

                // value.module.symbols = hashmap_init(8);
                value.module.functions = init_list(sizeof(struct AST *));
                // value.module.functions_map = hashmap_init(6);
                value.module.variables = init_list(sizeof(struct AST *));
                value.module.structures = init_list(sizeof(struct AST *));
                value.module.traits = init_list(sizeof(struct AST *));
                value.module.impls = init_list(sizeof(struct AST *));
            } break;
        case AST_SCOPE:
            {
                value.scope.nodes = init_list(sizeof(struct AST *));
                value.scope.symbol_table = symbol_table_init();
            } break;
        case AST_STRUCT:
            {
                value.structure.interner_id = INVALID_INTERN_ID;
                value.structure.generics = init_list(sizeof(String *));
                value.structure.functions = init_list(sizeof(struct AST *));
                value.structure.variables = init_list(sizeof(struct AST *));
            } break;
        case AST_TRAIT:
            {
                value.trait.children = init_list(sizeof(struct AST *));
                value.trait.implementers = init_list(sizeof(struct AST *));
            } break;
        case AST_FUNCTION:
        case AST_DECLARATION:
        case AST_EXPR:
        case AST_OP:
        case AST_VARIABLE:
        case AST_LITERAL:
        case AST_IMPL:
        case AST_RETURN:
        case AST_FOR:
        case AST_WHILE:
        case AST_IF:
            break;
        default:
            {
                println("Unsupported type: {s}({i})", ast_type_to_str(type), type);
                exit(1);
            }
    }

    return value;
}

#define ENUM_EL_TO_STR(el_name, str, ...) case el_name: return str;
const char * ast_type_to_str(enum AST_type type) {
    switch(type) {
        AST_FOREACH(ENUM_EL_TO_STR)
    }
    return "UNDEFINED";
}
#undef ENUM_EL_TO_STR

#define ENUM_EL_TO_UNION_SIZE(NAME, STR, TYPE) case NAME: if (sizeof(TYPE) == 0) { break; } else return sizeof(TYPE);
const size_t ast_union_size(enum AST_type type) {
    switch (type) {
        AST_FOREACH(ENUM_EL_TO_UNION_SIZE)
    }
    FATAL("Invalid type provided: {i}", type);
}
#undef ENUM_EL_TO_UNION_SIZE

const char * ast_type_to_str_ast(struct AST * ast) {
    if (ast == NULL)
        return "(NULL)";

    return ast_type_to_str(ast->type);
}

void free_ast(struct AST * ast) {

    free(ast);
}

void _print_ast_tree(struct AST * ast, String pad, char is_last) { 
    // pad->size == 2 means that it has only appended two
    if (pad.length != 0) {
        print("{2s}", pad._ptr, is_last ? "└─" : "├─");
    }

    if (ast == NULL) {
        println("(NULL)");
        return;
    }

    print_ast("{s}\n", ast);

    // create a new pointer so that this will not influence the next children
    pad = string_copy(pad);
    if (is_last) {
        string_append(&pad, "  ");
    } else {
        string_append(&pad, "│ ");
    }

    switch (ast->type) {
        case AST_ROOT:
            {
                a_root root = ast->value.root;

                String next_pad = string_copy(pad);
                AST_TREE_PRINT_CHILDREN_REVERSE(root.modules, next_pad);
                free_string(next_pad);

                break;
            }
        case AST_MODULE:
            {
#ifdef SHALLOW_PRINT
                break;
#endif
                a_module module = ast->value.module;

                String next_pad = string_copy(pad);
                AST_TREE_PRINT_CHILDREN(module.variables, next_pad);
                free_string(next_pad);

                next_pad = string_copy(pad);
                AST_TREE_PRINT_CHILDREN(module.impls, next_pad);
                free_string(next_pad);

                next_pad = string_copy(pad);
                AST_TREE_PRINT_CHILDREN(module.functions, next_pad);
                free_string(next_pad);

                break;
            }
        case AST_FUNCTION:
            {
                a_function func = ast->value.function;

                String next_pad = string_copy(pad);

                _print_ast_tree(func.arguments, next_pad, 0);
                _print_ast_tree(func.body, next_pad, 1);
                free_string(next_pad);

                break;
            }
        case AST_SCOPE:
            {
                a_scope scope = ast->value.scope;
                String next_pad = string_copy(pad);

                AST_TREE_PRINT_CHILDREN(scope.nodes, next_pad);
                free_string(next_pad);
            } break;
#ifdef IMPL_PRINT
        case AST_IMPL:
            {
                a_impl impl = ast->value.implementation;
                String * next_pad = string_copy(pad);

                AST_TREE_PRINT_CHILDREN(impl.members, next_pad);
                free_string(&next_pad);

            } break;
#endif
        case AST_DECLARATION:
            {
                a_declaration decl = ast->value.declaration;

                String next_pad = string_copy(pad);
                _print_ast_tree(decl.expression, next_pad, 1);
                free_string(next_pad);

                break;
            }
        case AST_EXPR:
            {
                a_expr expr = ast->value.expression;

                String next_pad = string_copy(pad);
                AST_TREE_PRINT_CHILDREN(expr.children, next_pad);
                free_string(next_pad);

                break;
            }
        case AST_OP:
            {
                a_op op = ast->value.operator;

                String next_pad = string_copy(pad);
                if (op.op->mode == BINARY) {
                    _print_ast_tree(op.left, next_pad, 0);
                    _print_ast_tree(op.right, next_pad, 1);
                } else {
                    string_append(&next_pad, PADDING_DIRECT_CHILD);
                    _print_ast_tree(op.right, next_pad, 1);
                }
                free_string(next_pad);

                break;
            }
        case AST_RETURN:
            {
                a_return ret = ast->value.return_statement;

                if (ret.expression) {
                    String next_pad = string_copy(pad);
                    _print_ast_tree(ret.expression, next_pad, 1);
                    free_string(next_pad);
                }

                break;
            }
        case AST_FOR:
            {
                a_for_statement for_statement = ast->value.for_statement;

                String next_pad = string_copy(pad);
                _print_ast_tree(for_statement.expression, next_pad, 0);
                _print_ast_tree(for_statement.body, next_pad, 1);
                free_string(next_pad);

                break;
            }
        case AST_WHILE:
            {
                a_while_statement while_statement = ast->value.while_statement;

                String next_pad = string_copy(pad);
                _print_ast_tree(while_statement.expression, next_pad, 0);
                _print_ast_tree(while_statement.body, next_pad, 1);
                free_string(next_pad);

                break;
            }
        case AST_IF:
            {
                a_if_statement if_statement = ast->value.if_statement;

                String next_pad = string_copy(pad);

                while (1) {
                    if (if_statement.expression) {
                        _print_ast_tree(if_statement.expression, next_pad, 0);
                        _print_ast_tree(if_statement.body, next_pad, if_statement.next == NULL);
                    } else {
                        _print_ast_tree(if_statement.body, next_pad, 1);
                    }
                    if (if_statement.next == NULL)
                        break;
                    if_statement = *if_statement.next;
                }
                free_string(next_pad);

                break;
            }
    }
}

void print_ast_tree(struct AST * ast) {
    String string = init_string("");
    _print_ast_tree(ast, string, 1);
    free_string(string);
}

char * ast_to_string(struct AST * ast) {
    const char * type_str = ast_type_to_str_ast(ast);
    const char * scope = ast_type_to_str_ast(ast->scope);

    char * ast_str = format(RED "{s}" RESET ": ", type_str);

    switch (ast->type) {
        case AST_MODULE:
            {
                a_module module = ast->value.module;
                ast_str = format("{s} " GREY "<" BLUE "Symbols" RESET ": {i}, " BLUE "Path" RESET ": {s}" GREY ">" RESET, ast_str, 0 /*module.symbols->total */, module.path);
                break;
            }
        case AST_FUNCTION:
            {
                a_function func = ast->value.function;
                const char * func_name = interner_lookup_str(func.interner_id)._ptr;
                ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {2s: -> }" GREY ">" RESET, ast_str, func_name, get_type_str(func.param_type), get_type_str(func.return_type));
                break;
            }
        case AST_SCOPE:
            {
                a_scope scope = ast->value.scope;
                ast_str = format("{s} " GREY "<" BLUE "Variables" RESET ": {i}, " BLUE "Nodes" RESET ": {i}" GREY ">" RESET, ast_str, scope.symbol_table.entries.size, scope.nodes->size);
            } break;
        case AST_IMPL:
            {
                a_impl impl = ast->value.implementation;
                const char * impl_name = interner_lookup_str(impl.interner_id)._ptr;
                ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Types" RESET ": {s}" GREY ">" RESET, ast_str, impl_name, get_type_str(impl.type));
            } break;
        case AST_OP:
            {
                a_op op = ast->value.operator;
                ast_str = format("{s} " GREY "<" BLUE "Op" RESET ": '{s}', " BLUE "Mode" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, op.op ? op.op->str : "(NULL)", op.op->mode == BINARY ? "Binary" : "Unary", get_type_str(op.type));
                break;
            }
        case AST_VARIABLE:
            {
                a_variable var = ast->value.variable;
                const char * var_name = interner_lookup_str(var.interner_id)._ptr;
                ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, var_name, get_type_str(var.type));
                break;
            }
        case AST_LITERAL:
            {
                a_literal literal = ast->value.literal;
                const char * fmt_string;

                switch (literal.literal_type) {
                    case LITERAL_NUMBER:
                        fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": Number, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": {s}" GREY ">" RESET;
                        break;
                    case LITERAL_STRING:
                        fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": String, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": \"{s}\"" GREY ">" RESET;
                        break;
                }

                ast_str = format(fmt_string, ast_str, type_to_str(*literal.type), literal.value);
                break;
            }
        case AST_DECLARATION:
            {
                a_declaration declaration = ast->value.declaration;
                ast_str = format("{s} " GREY "<" BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, declaration.is_const ? "Constant" : "Variable");
                break;
            }
        default:
            {
                ast_str = format("{s} " GREY "<>" RESET, ast_str);
                break;
            }
    }

    return ast_str;
}

void print_ast(const char * template, struct AST * ast) {
    char * ast_str = ast_to_string(ast);
    print(template, ast_str);
    free(ast_str);
}

struct AST * get_scope(enum AST_type type, struct AST * scope) {
    while (scope->type != AST_ROOT && scope->type != type) {
        scope = scope->scope;
    }

    return scope;
}
