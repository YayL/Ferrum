#pragma once

#include "lexer.h"
#include "keywords.h"
#include "operators.h"
#include "deque.h"
#include "AST.h"

typedef struct Parser {
    struct Lexer * lexer;
    struct Token * token;
    struct Token * prev;
} * Parser;

struct Parser * init_parser(struct Lexer * lexer);
struct Token * nth_token(struct Parser * parser, int offset);
struct Token * next_token(struct Parser * parser);

unsigned int is_statement(char *);

void parser_eat(struct Parser * parser, enum token_t type);

struct Ast * parser_parse(struct Parser * parser);

struct Ast * parser_parse_if(struct Parser * parser);
struct Ast * parser_parse_for(struct Parser * parser);
struct Ast * parser_parse_while(struct Parser * parser);
struct Ast * parser_parse_do(struct Parser * parser);
struct Ast * parser_parse_match(struct Parser * parser);
struct Ast * parser_parse_return(struct Parser * parser);

struct Ast * parser_parse_function(struct Parser * parser);
struct Ast * parser_parse_package(struct Parser * parser);
struct Ast * parser_parse_id(struct Parser * parser);
struct Ast * parser_parse_operator(struct Parser * parser);

struct Ast * parser_parse_int(struct Parser * parser);
struct Ast * parser_parse_operator(struct Parser * parser);
struct Ast * parser_parse_statement(struct Parser * parser);

struct Ast * parser_parse_identifier(struct Parser * parser);
struct Ast * parser_parse_expr(struct Parser * parser);

struct Ast * parser_parse_scope(struct Parser * parser);
struct Ast * parser_parse_module(struct Parser * parser);
