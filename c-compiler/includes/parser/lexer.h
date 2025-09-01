#pragma once

#include "parser/token.h"

struct Lexer {
    char * src;
    struct Token tok;
    size_t index, size;
    unsigned int line, pos;
    char c;
};

struct Lexer lexer_init(const char * file_path);
void lexer_free(struct Lexer * lexer);

void lexer_update_token(struct Lexer * lexer, const char * start, size_t length, enum token_t type);

void lexer_advance(struct Lexer * lexer);
void lexer_skip_whitespace(struct Lexer * lexer);

void lexer_advance_current(struct Lexer * lexer, enum token_t type);

void lexer_update(struct Lexer * lexer, unsigned int increment);
char lexer_peek(struct Lexer * lexer, unsigned int offset);

void lexer_parse_id(struct Lexer * lexer);
void lexer_parse_int(struct Lexer * lexer);
void lexer_parse_string(struct Lexer * lexer);
void lexer_parse_single_line_comment(struct Lexer * lexer);
void lexer_parse_multi_line_comment(struct Lexer * lexer);
void lexer_next_token(struct Lexer * lexer);
void lexer_parse_operator(struct Lexer * lexer);
