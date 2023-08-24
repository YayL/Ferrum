#pragma once

#include "parser/token.h"
#include "common/common.h"

struct Lexer {
    char * src;
    size_t index, size;
    char c;
    unsigned int line, pos;
    struct Token * tok;
};

struct Lexer * init_lexer(char * src, size_t length);

void lexer_advance(struct Lexer * lexer);
void lexer_skip_whitespace(struct Lexer * lexer);

void lexer_advance_current(struct Lexer * lexer, enum token_t type);

char lexer_peek(struct Lexer * lexer, unsigned int offset);

void lexer_parse_id(struct Lexer * lexer);
void lexer_parse_int(struct Lexer * lexer);
void lexer_parse_string(struct Lexer * lexer);
void lexer_parse_single_line_comment(struct Lexer * lexer);
void lexer_parse_multi_line_comment(struct Lexer * lexer);
void lexer_next_token(struct Lexer * lexer);
void lexer_parse_operator(struct Lexer * lexer);
