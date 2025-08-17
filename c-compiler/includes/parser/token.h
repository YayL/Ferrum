#pragma once

#include "common/sourcespan.h"
#include "common/string.h"
#include "tables/interner.h"

struct Token {
	union token_value_union {
		unsigned int interner_id;
		SourceSpan span;
	} value;

	unsigned int line, pos;
	enum token_t {
		TOKEN_EOF,
		TOKEN_ID,
		TOKEN_OP,

		TOKEN_PLUS,
		TOKEN_MINUS,
		TOKEN_EQUAL,
		TOKEN_LT,
		TOKEN_GT,
		TOKEN_LPAREN,
		TOKEN_RPAREN,
		TOKEN_UNDERSCORE,
		TOKEN_LBRACKET,
		TOKEN_RBRACKET,
		TOKEN_LBRACE,
		TOKEN_RBRACE,
		TOKEN_COLON,
		TOKEN_SEMI,
		TOKEN_COMMA,
		TOKEN_ASTERISK,
		TOKEN_CARET,
		TOKEN_AMPERSAND,
		TOKEN_TILDA,
		TOKEN_DOT,
		TOKEN_PERCENT,
		TOKEN_EXCLAMATION_MARK,
		TOKEN_QUESTION_MARK,
		TOKEN_VERTICAL_LINE,
		TOKEN_BACKSLASH,
		TOKEN_SLASH,

		TOKEN_INT,
		TOKEN_STRING_LITERAL,

		TOKEN_COMMENT,
		TOKEN_LINE_BREAK,
	} type;
};

struct Token * init_token();
void set_token(Interner * intern, struct Token * tok, char * value, unsigned int length, enum token_t type, unsigned int line, unsigned int pos);
void copy_token(struct Token * dest, struct Token * src);

const char* token_type_to_str(enum token_t type);
char * token_to_str(struct Token token);
