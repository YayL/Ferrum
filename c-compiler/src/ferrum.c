#include "ferrum.h"

#include "io.h"
#include "lexer.h"
#include "parser.h"

void _indent(size_t indent) {
    for (size_t i = 0; i < indent; ++i)
        print(" | ");
}

void print_root(struct Ast * node, size_t indent) {
    if (indent == 0)
        print_ast("{s}\n", node);

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

}

void ferrum_compile(char * src, size_t length) {

    struct Lexer * lexer = init_lexer(src, length);

    struct Parser * parser = init_parser(lexer);
    struct Ast * root = parser_parse_module(parser);

    print_root(root, 0);

}

void ferrum_compile_file(char * path) {
    
    size_t length;
    char * src = read_file(path, &length);
    
    ferrum_compile(src, length);
    free(src);
}

