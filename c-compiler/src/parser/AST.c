#include "parser/AST.h"

#include "tables/interner.h"
#include "tables/registry_manager.h"

ID ast_get_scope_id(ID node_id) {
    return LOOKUP(node_id, struct AST_info).scope_id;
}

void ast_init_node(enum id_type type, void * node_ref) {
    switch (type) {
        // case ID_AST_ROOT:
        //     ((a_root *) node_ref)->modules = kh_init(map_string_to_id);
        //     break;
        case ID_AST_MODULE:
            ((a_module *) node_ref)->members = arena_init(sizeof(ID));
            ((a_module *) node_ref)->sym_table = symbol_table_init();
            ((a_module *) node_ref)->file_path = NULL;
            break;
        case ID_AST_SCOPE:
            ((a_scope *) node_ref)->nodes = arena_init(sizeof(ID));
            ((a_scope *) node_ref)->declarations = arena_init(sizeof(ID));
            break;
        case ID_AST_STRUCT:
            ((a_structure *) node_ref)->declarations = arena_init(sizeof(ID));
            ((a_structure *) node_ref)->name_id = INVALID_ID;
            break;
        case ID_AST_TRAIT:
            ((a_trait *) node_ref)->children = arena_init(sizeof(ID));
            ((a_trait *) node_ref)->implementations = arena_init(sizeof(ID));
            ((a_trait *) node_ref)->where = arena_init(sizeof(ID));
            ((a_trait *) node_ref)->name_id = INVALID_ID;
            break;
        case ID_AST_IMPL:
            ((a_implementation *) node_ref)->members = arena_init(sizeof(ID));
            ((a_implementation *) node_ref)->where = arena_init(sizeof(ID));
            ((a_implementation *) node_ref)->trait_symbol_id = INVALID_ID;
            break;
        case ID_AST_SYMBOL:
            ((a_symbol *) node_ref)->name_ids = arena_init(sizeof(ID));
            ((a_symbol *) node_ref)->node_id = INVALID_ID;
            break;
        case ID_AST_OP:
            ((a_operator *) node_ref)->definition.function_id = INVALID_ID;
            ((a_operator *) node_ref)->left_id = INVALID_ID;
            ((a_operator *) node_ref)->right_id = INVALID_ID;
            ((a_operator *) node_ref)->type_id = INVALID_ID;
            ((a_operator *) node_ref)->op = operator_get(OP_NOT_FOUND);
        case ID_AST_GROUP:
        case ID_AST_VARIABLE:
        case ID_AST_IMPORT:
        case ID_AST_FUNCTION:
        case ID_AST_DECLARATION:
        case ID_AST_EXPR:
        case ID_AST_LITERAL:
        case ID_AST_RETURN:
        case ID_AST_FOR:
        case ID_AST_WHILE:
        case ID_AST_IF:
            break;
        default:
            {
                FATAL("Unsupported type: {s}({i})", id_type_to_string(type), type);
                exit(1);
            }
    }
}

#define PADDING_DIRECT_CHILD  ""
#define PADDING_LIST_CHILDREN "│"
#define ID_AST_TREE_PADDING(comp) (comp ? PADDING_LIST_CHILDREN : PADDING_DIRECT_CHILD)
#define ID_AST_TREE_PRINT_CHILDREN(arena, pstring) {for (int i = 0; i < arena.size; ++i){ _print_ast_tree(ARENA_GET(arena, i, ID), pstring, i == (arena.size - 1));}}
#define ID_AST_TREE_PRINT_CHILDREN_REVERSE(list, pstring) {for (ssize_t i = list->size - 1; 0 <= i; --i){ _print_ast_tree(list_at(list, i), pstring, i == 0);}}

void _print_ast_tree(ID node_id, String pad, char is_last) { 
    // pad->size == 2 means that it has only appended two
    if (pad.length != 0) {
        print("{2s}", pad._ptr, is_last ? "└─" : "├─");
    }

    if (ID_IS_INVALID(node_id)) {
        println("(NULL)");
        return;
    }

    char * ast_str = ast_to_string(node_id);
    puts(ast_str);
    free(ast_str);

    // create a new pointer so that this will not influence the next children
    pad = string_copy(pad);
    if (is_last) {
        string_append(&pad, "  ");
    } else {
        string_append(&pad, "│ ");
    }

    switch (node_id.type) {
        case ID_AST_ROOT: {
            a_root root = LOOKUP(node_id, a_root);
            ASSERT1(id_is_equal(root.info.node_id, node_id));

            String next_pad = string_copy(pad);
            ID child_node_id;

            unsigned int counter = 0;
            kh_foreach_value(&root.modules, child_node_id, {
                _print_ast_tree(child_node_id, next_pad, ++counter == kh_size(&root.modules));
            })
            free_string(next_pad);
        } break;

        case ID_AST_MODULE: {
#ifdef SHALLOW_PRINT
            break;
#endif
            a_module module = LOOKUP(node_id, a_module);
            ASSERT1(id_is_equal(module.info.node_id, node_id));

            String next_pad = string_copy(pad);
            ID_AST_TREE_PRINT_CHILDREN(module.members, next_pad);
            free_string(next_pad);
        } break;

        case ID_AST_FUNCTION: {
            a_function func = LOOKUP(node_id, a_function);

            String next_pad = string_copy(pad);

            _print_ast_tree(func.arguments_id, next_pad, 0);
            _print_ast_tree(func.body_id, next_pad, 1);
            free_string(next_pad);
        } break;

        case ID_AST_SCOPE: {
            a_scope scope = LOOKUP(node_id, a_scope);
            String next_pad = string_copy(pad);

            ID_AST_TREE_PRINT_CHILDREN(scope.nodes, next_pad);
            free_string(next_pad);
        } break;

#ifdef IMPL_PRINT
        case ID_AST_IMPL: {
            a_impl impl = ast->value.implementation;
            String * next_pad = string_copy(pad);

            ID_AST_TREE_PRINT_CHILDREN(impl.members, next_pad);
            free_string(&next_pad);

        } break;
#endif

        case ID_AST_DECLARATION: {
            a_declaration decl = LOOKUP(node_id, a_declaration);

            String next_pad = string_copy(pad);
            _print_ast_tree(decl.expression_id, next_pad, 1);
            free_string(next_pad);
        } break;

        case ID_AST_EXPR: {
            a_expression expr = LOOKUP(node_id, a_expression);

            String next_pad = string_copy(pad);
            ID_AST_TREE_PRINT_CHILDREN(expr.children, next_pad);
            free_string(next_pad);
        } break;

        case ID_AST_OP: {
            a_operator op = LOOKUP(node_id, a_operator);

            String next_pad = string_copy(pad);
            if (op.op.mode == BINARY) {
                _print_ast_tree(op.left_id, next_pad, 0);
                _print_ast_tree(op.right_id, next_pad, 1);
            } else {
                string_append(&next_pad, PADDING_DIRECT_CHILD);
                _print_ast_tree(op.right_id, next_pad, 1);
            }
            free_string(next_pad);
        } break;

        case ID_AST_RETURN: {
            a_return_statement ret = LOOKUP(node_id, a_return_statement);

            if (!ID_IS_INVALID(ret.expression_id)) {
                String next_pad = string_copy(pad);
                _print_ast_tree(ret.expression_id, next_pad, 1);
                free_string(next_pad);
            }
        } break;

        case ID_AST_FOR: {
            a_for_statement for_statement = LOOKUP(node_id, a_for_statement);

            String next_pad = string_copy(pad);
            _print_ast_tree(for_statement.expression_id, next_pad, 0);
            _print_ast_tree(for_statement.body_id, next_pad, 1);
            free_string(next_pad);
        } break;

        case ID_AST_WHILE: {
            a_while_statement while_statement = LOOKUP(node_id, a_while_statement);

            String next_pad = string_copy(pad);
            _print_ast_tree(while_statement.expression_id, next_pad, 0);
            _print_ast_tree(while_statement.body_id, next_pad, 1);
            free_string(next_pad);
        } break;

        case ID_AST_IF: {
            a_if_statement if_statement = LOOKUP(node_id, a_if_statement);

            String next_pad = string_copy(pad);

            while (1) {
                if (!ID_IS_INVALID(if_statement.expression_id)) {
                    _print_ast_tree(if_statement.expression_id, next_pad, 0);
                    _print_ast_tree(if_statement.body_id, next_pad, ID_IS_INVALID(if_statement.next_id));
                } else {
                    _print_ast_tree(if_statement.body_id, next_pad, 1);
                }
                if (ID_IS_INVALID(if_statement.next_id))
                    break;

                if_statement = LOOKUP(if_statement.next_id, a_if_statement);
            }
            free_string(next_pad);
        } break;

        case ID_AST_SYMBOL: {
            a_symbol * symbol = lookup(node_id);

            if (!ID_IS_INVALID(symbol->node_id)) {
                String next_pad = string_copy(pad);
                _print_ast_tree(symbol->node_id, next_pad, 1);
                free_string(next_pad);
            }

        } break;
    }
}

void print_ast_tree(ID node_id) {
    String string = string_init("");
    _print_ast_tree(node_id, string, 1);
    free_string(string);
}

void print_ast_tree_from_root(a_root root) {
    println(RED "Root" RESET ": " GREY "<" BLUE "Modules" RESET ": {u}" GREY ">" RESET, root.modules.size);

    String next_pad = string_init(" ");
    ID child_node_id;

    unsigned int counter = 0;
    kh_foreach_value(&root.modules, child_node_id, {
        _print_ast_tree(child_node_id, next_pad, ++counter == kh_size(&root.modules));
    })
    free_string(next_pad);
}

char * ast_to_string(ID node_id) {
    const char * type_str = id_type_to_string(node_id.type);
    const char * scope = id_type_to_string(ast_get_scope_id(node_id).type);

    char * ast_str = format(RED "{s}" RESET ": ", type_str);

    switch (node_id.type) {
        case ID_AST_MODULE: {
            a_module module = LOOKUP(node_id, a_module);
            ast_str = format("{s} " GREY "<" BLUE "Definitions" RESET ": {i}, " BLUE "Path" RESET ": {s}" GREY ">" RESET, ast_str, module.members.size, module.file_path);
        } break;

        case ID_AST_FUNCTION: {
            a_function func = LOOKUP(node_id, a_function);
            const char * func_name = interner_lookup_str(func.name_id)._ptr;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {2s: -> }" GREY ">" RESET, ast_str, func_name, type_to_str(func.param_type), type_to_str(func.return_type));
        } break;

        case ID_AST_SCOPE: {
            a_scope scope = LOOKUP(node_id, a_scope);
            ast_str = format("{s} " GREY "<" BLUE "Declarations" RESET ": {i}, " BLUE "Nodes" RESET ": {i}" GREY ">" RESET, ast_str, scope.declarations.size, scope.nodes.size);
        } break;

        case ID_AST_IMPL: {
            a_implementation impl = LOOKUP(node_id, a_implementation);

            ASSERT1(ID_IS(impl.trait_symbol_id, ID_AST_SYMBOL));
            a_symbol symbol = LOOKUP(impl.trait_symbol_id, a_symbol);
            const char * impl_name = interner_lookup_str(symbol.name_id)._ptr;

            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}" GREY ">" RESET, ast_str, impl_name);
        } break;

        case ID_AST_TRAIT: {
            a_trait trait = LOOKUP(node_id, a_trait);
            ASSERT1(!ID_IS_INVALID(trait.name_id));
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Templates" RESET ": {u}" GREY ">" RESET, ast_str, interner_lookup_str(trait.name_id)._ptr, trait.templates.size);
        } break;

        case ID_AST_OP: {
            a_operator op = LOOKUP(node_id, a_operator);
            ast_str = format("{s} " GREY "<" BLUE "Op" RESET ": '{s}', " BLUE "Mode" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, op.op.str, op.op.mode == BINARY ? "Binary" : "Unary", type_to_str(op.type_id));
        } break;

        case ID_AST_VARIABLE: {
            a_variable var = LOOKUP(node_id, a_variable);
            const char * var_name = interner_lookup_str(var.name_id)._ptr;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, var_name, type_to_str(var.type_id));
        } break;

        case ID_AST_GROUP: {
            a_group group = LOOKUP(node_id, a_group);
            const char * group_name = interner_lookup_str(group.name_id)._ptr;
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Type" RESET ": {s}" GREY ">" RESET, ast_str, group_name, type_to_str(group.type_id));
        } break;

        case ID_AST_LITERAL: {
            a_literal literal = LOOKUP(node_id, a_literal);
            const char * fmt_string;

            switch (literal.literal_type) {
                case LITERAL_NUMBER:
                    fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": Number, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": {s}" GREY ">" RESET;
                    break;
                case LITERAL_STRING:
                    fmt_string = "{s} " GREY "<" BLUE "Literal" RESET ": String, " BLUE "Type" RESET ": {s}, " BLUE "Value" RESET ": \"{s}\"" GREY ">" RESET;
                    break;
            }

            ast_str = format(fmt_string, ast_str, type_to_str(literal.type_id), literal.value);
        } break;

        case ID_AST_IMPORT: {
            a_import import = LOOKUP(node_id, a_import);
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}" GREY ">" RESET, ast_str, interner_lookup_str(import.name_id));
        } break;

        case ID_AST_SYMBOL: {
            a_symbol symbol = LOOKUP(node_id, a_symbol);
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Found" RESET ": {b}" GREY ">" RESET, ast_str, interner_lookup_str(symbol.name_id)._ptr, !ID_IS_INVALID(symbol.node_id));
        } break;

        case ID_AST_STRUCT: {
            a_structure structure = LOOKUP(node_id, a_structure);
            ast_str = format("{s} " GREY "<" BLUE "Name" RESET ": {s}, " BLUE "Declarations" RESET ": {u}" GREY ">" RESET, ast_str, interner_lookup_str(structure.name_id)._ptr, structure.declarations.size);
        } break;

        default: {
            ast_str = format("{s} " GREY "<>" RESET, ast_str);
        } break;
    }

    return ast_str;
}

void print_ast(const char * template, ID node_id) {
    char * ast_str = ast_to_string(node_id);
    print(template, ast_str);
    free(ast_str);
}

void * get_scope(enum id_type type, ID scope_id) {
    void * node = NULL;

    while (!ID_IS(scope_id, ID_AST_ROOT) && !ID_IS(scope_id, type)) {
        node = lookup(scope_id);
        scope_id = ((struct AST_info *) node)->scope_id;
    }

    node = lookup(scope_id);

    ASSERT1(ID_IS(((struct AST_info *)node)->node_id, type));
    return node;
}
