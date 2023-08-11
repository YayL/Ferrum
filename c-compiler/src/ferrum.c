#include "ferrum.h"

#include "common/io.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "codegen/checker.h"

void _indent(size_t indent) {
    for (size_t i = 0; i < indent; ++i)
        print(" | ");
}

void print_root(struct Ast * node, size_t indent) {
    if (indent == 0)
        print_ast("{s}\n", node);

    if (node->nodes) {
        struct Ast * temp;
        for (int i = 0; i < node->nodes->size; ++i) {
            _indent(indent);
            temp = list_at(node->nodes, i);
            if (temp == NULL) {
                println("NULL");
                continue;
            }
            print_ast("*|{s}\n", temp);
            print_root(temp, indent + 1);
        }
    }

    if (node->left) {
        _indent(indent);
        print_ast("L|{s}\n", node->left);
        print_root(node->left, indent + 1);
    }
    if (node->value) {
        _indent(indent);
        print_ast("V|{s}\n", node->value);
        print_root(node->value, indent + 1);
    }
    if (node->right) {
        _indent(indent);
        print_ast("V|{s}\n", node->right);
        print_root(node->value, indent + 1);
    }

}

void ferrum_compile(char * file_path) {

    struct Ast * root = init_ast(AST_ROOT);
    root->nodes = init_list(sizeof(struct Ast *));
    
    char * abs_path = get_abs_path(file_path);

    parser_parse(root, abs_path);
    checker_check(root);

    print_root(root, 0);

    println("\nFinished!");
}

