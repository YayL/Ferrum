#include "parser/token.h"

#include "common/sourcespan.h"
#include "common/string.h"
#include "fmt.h"
#include "tables/interner.h"

struct Token * init_token() {
	struct Token * token = calloc(1, sizeof(struct Token));
	return token;
}

void set_token(Interner * intern, struct Token * tok, char * value, unsigned int length, enum token_t type, unsigned int line, unsigned int pos) {
	switch (type) {
	case TOKEN_ID:
		tok->value.interner_id = interner_intern(init_string_with_length(value, length)); break;
	default:
		tok->value.span = source_span_init(value, length); break;	
	}

	tok->type = type;
	tok->line = line;
	tok->pos = pos;
}

void copy_token(struct Token * dest, struct Token * src) {
	memcpy(dest, src, sizeof(*dest));
}

const char* token_type_to_str(enum token_t type) {
	switch(type) {
		case TOKEN_EOF: return "TOKEN_EOF";
		case TOKEN_ID: return "TOKEN_ID";
		case TOKEN_OP: return "TOKEN_OP";
		case TOKEN_PLUS: return "TOKEN_PLUS";
		case TOKEN_MINUS: return "TOKEN_MINUS";
		case TOKEN_EQUAL: return "TOKEN_EQUAL";
		case TOKEN_LT: return "TOKEN_LT";
		case TOKEN_GT: return "TOKEN_GT";
		case TOKEN_LPAREN: return "TOKEN_LPAREN";
		case TOKEN_RPAREN: return "TOKEN_RPAREN";
		case TOKEN_UNDERSCORE: return "TOKEN_UNDERSCORE";
		case TOKEN_LBRACKET: return "TOKEN_LBRACKET";
		case TOKEN_RBRACKET: return "TOKEN_RBRACKET";
		case TOKEN_LBRACE: return "TOKEN_LBRACE";
		case TOKEN_RBRACE: return "TOKEN_RBRACE";
		case TOKEN_COLON: return "TOKEN_COLON";
		case TOKEN_SEMI: return "TOKEN_SEMI";
		case TOKEN_COMMA: return "TOKEN_COMMA";
		case TOKEN_ASTERISK: return "TOKEN_ASTERISK";
		case TOKEN_CARET: return "TOKEN_CARET";
		case TOKEN_AMPERSAND: return "TOKEN_AMPERSAND";
		case TOKEN_TILDA: return "TOKEN_TILDA";
		case TOKEN_DOT: return "TOKEN_DOT";
		case TOKEN_PERCENT: return "TOKEN_PERCENT";
		case TOKEN_EXCLAMATION_MARK: return "TOKEN_EXCLAMATION_MARK";
		case TOKEN_QUESTION_MARK: return "TOKEN_QUESTION_MARK";
		case TOKEN_VERTICAL_LINE: return "TOKEN_VERTICAL_LINE";
		case TOKEN_BACKSLASH: return "TOKEN_BACKSLASH";
		case TOKEN_SLASH: return "TOKEN_SLASH";

		case TOKEN_INT: return "TOKEN_INT";
		case TOKEN_STRING_LITERAL: return "TOKEN_STRING_LITERAL";

		case TOKEN_COMMENT: return "TOKEN_COMMENT";
		case TOKEN_LINE_BREAK: return "TOKEN_LINE_BREAK";
	}
	return "UNDEFINED";
}

char * token_to_str(Interner interner, struct Token token) {
	const char * type_str = token_type_to_str(token.type);

	const char * value;
	switch (token.type) {
		case TOKEN_ID:
			value = interner_lookup_str(token.value.interner_id)._ptr; break;
		default:
			value = source_span_to_cstr(token.value.span); break;
	}

	return format("{2u::} <type='{s}', code='{u}', value='{s}'>", token.line, token.pos, type_str, token.type, value);
}
