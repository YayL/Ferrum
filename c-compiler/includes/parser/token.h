#pragma once

struct Token {
	char * value;
    unsigned int length;
	enum token_t {
		TOKEN_ID,

		TOKEN_SEMI,
		TOKEN_LBRACE,
		TOKEN_RBRACE,
		TOKEN_COLON,
		TOKEN_COMMA,
        TOKEN_OP,
		
        TOKEN_INT,
		TOKEN_STRING_LITERAL,
        TOKEN_SINGLE_QUOTE,
        TOKEN_BACKSLASH,
		
		TOKEN_COMMENT,
        TOKEN_LINE_BREAK,
		TOKEN_EOF,
	} type;
	unsigned int line, pos;
};

struct Token * init_token(char * content, unsigned int length, enum token_t type, unsigned int line, unsigned int pos);
const char* token_type_to_str(enum token_t type);
void print_token(const char * template, struct Token * token);
