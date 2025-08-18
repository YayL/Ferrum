#pragma once

#include "parser/keywords.h"
#include "codegen/AST.h"

typedef struct Parser {
    struct Lexer * lexer;

    struct AST * root;
    struct AST * current_scope;

    char * path;

    struct Token * token;

    struct Token prev;
    char error;
} * Parser;

struct Parser * init_parser(char * path);
struct Token * nth_token(struct Parser * parser, int offset);
struct Token * next_token(struct Parser * parser);

unsigned int is_statement(char *);

void parser_eat(struct Parser * parser, enum token_t type);

void parser_parse(struct AST * root, char * path);

Type parser_parse_type(struct Parser * parser);

struct AST * parser_parse_if(struct Parser * parser);
struct AST * parser_parse_for(struct Parser * parser);
struct AST * parser_parse_while(struct Parser * parser);
struct AST * parser_parse_do(struct Parser * parser);
struct AST * parser_parse_match(struct Parser * parser);
struct AST * parser_parse_return(struct Parser * parser);
struct AST * parser_parse_declaration(struct Parser * parser, enum Keywords keyword);

struct AST * parser_parse_function(struct Parser * parser);
struct AST * parser_parse_package(struct Parser * parser);
struct AST * parser_parse_id(struct Parser * parser);
struct AST * parser_parse_operator(struct Parser * parser);

struct AST * parser_parse_int(struct Parser * parser);
struct AST * parser_parse_string(struct Parser * parser);
struct AST * parser_parse_operator(struct Parser * parser);
struct AST * parser_parse_statement(struct Parser * parser);

struct AST * parser_parse_identifier(struct Parser * parser);
struct AST * parser_parse_expr_exit_on(struct Parser * parser, enum Operators op);
struct AST * parser_parse_expr(struct Parser * parser);
struct AST * parser_parse_statement_expr(struct Parser * parser);

struct AST * parser_parse_scope(struct Parser * parser);
struct AST * parser_parse_module(struct Parser * parser, struct AST * module);
