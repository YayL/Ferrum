#pragma once

#include "token.h"
#include "common.h"

struct Lexer {
    char * src;
    size_t index, size;
    char c;
    unsigned int line, pos;
};

struct Lexer * init_lexer(char * src, size_t length);

void lexer_advance(struct Lexer * lexer);
void lexer_skip_whitespace(struct Lexer * lexer);

struct Token * lexer_advance_current(struct Lexer * lexer, enum token_t type);
struct Token * lexer_advance_with(struct Lexer * lexer, struct Token * token);

char lexer_peek(struct Lexer * lexer, unsigned int offset);

struct Token * lexer_parse_id(struct Lexer * lexer);
struct Token * lexer_parse_int(struct Lexer * lexer);
struct Token * lexer_parse_string(struct Lexer * lexer);
struct Token * lexer_parse_single_line_comment(struct Lexer * lexer);
struct Token * lexer_parse_multi_line_comment(struct Lexer * lexer);
struct Token * lexer_next_token(struct Lexer * lexer);
struct Token * lexer_parse_operator(struct Lexer * lexer);
