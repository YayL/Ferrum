#pragma once



struct Token {
	char * value;
    unsigned int length;
	enum token_t {
        TOKEN_EOF,
		TOKEN_ID,
        TOKEN_OP,

        TOKEN_PLUS,
        TOKEN_MINUS,
        TOKEN_EQUAL,
        TOKEN_LESS_THAN,
        TOKEN_GREATER_THAN,
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
	unsigned int line, pos;
};

struct Token * init_token();
void set_token(struct Token * tok, char * value, unsigned int length, enum token_t type, unsigned int line, unsigned int pos);
void copy_token(struct Token * dest, struct Token * src);

const char* token_type_to_str(enum token_t type);
void print_token(const char * template, struct Token * token);
