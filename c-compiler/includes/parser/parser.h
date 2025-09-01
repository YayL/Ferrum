#pragma once

#include "parser/AST.h"
#include "parser/keywords.h"
#include "parser/token.h"
#include "parser/lexer.h"

typedef struct Parser {
    a_root * root;
    char * path;

    struct Lexer lexer;
    ID current_scope_id;

    Arena modules_to_parse;

    struct Token previous_token;
    char error;
} Parser;

struct Parser init_parser(char * path);
struct Token * nth_token(struct Parser * parser, int offset);
struct Token * next_token(struct Parser * parser);

unsigned int is_statement(char *);

void parser_eat(struct Parser * parser, enum token_t type);

void parser_parse(a_root * root, char * path);

ID parser_parse_type(struct Parser * parser);

ID parser_parse_if(struct Parser * parser);
ID parser_parse_for(struct Parser * parser);
ID parser_parse_while(struct Parser * parser);
ID parser_parse_do(struct Parser * parser);
ID parser_parse_match(struct Parser * parser);
ID parser_parse_return(struct Parser * parser);
ID parser_parse_declaration(struct Parser * parser, enum Keywords keyword);

ID parser_parse_function(struct Parser * parser);
ID parser_parse_import(struct Parser * parser);
ID parser_parse_id(struct Parser * parser);
ID parser_parse_symbol(struct Parser * parser);
ID parser_parse_operator(struct Parser * parser);

ID parser_parse_int(struct Parser * parser);
ID parser_parse_string(struct Parser * parser);
ID parser_parse_operator(struct Parser * parser);
ID parser_parse_statement(struct Parser * parser);

ID parser_parse_identifier(struct Parser * parser);
ID parser_parse_expr_exit_on(struct Parser * parser, enum Operators op);
ID parser_parse_expr(struct Parser * parser);
ID parser_parse_statement_expr(struct Parser * parser);

ID parser_parse_scope(struct Parser * parser);
void parser_parse_module(struct Parser * parser, a_module * module);
