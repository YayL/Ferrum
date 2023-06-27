#include "lexer.h"

#include "operators.h"
#include <ctype.h>


struct Lexer * init_lexer(char * src, size_t length) {

    struct Lexer * lexer = malloc(sizeof(struct Lexer));

    lexer->src = src;
    lexer->size = length;
    lexer->c = src[0];
    lexer->pos = lexer->line = 1;
    lexer->index = 0;

    return lexer;
}


void lexer_advance(struct Lexer * lexer) {

    if (lexer->index < lexer->size && lexer->c != EOF) {
        lexer->c = lexer->src[++lexer->index];
        lexer->pos++;
        return;
    }

    println("[Lexer]: End of file found while lexing: ");
    println("Index = {u}, Size = {u}, C = {c}({i})", lexer->index, lexer->size, lexer->c, lexer->c);
    for (unsigned int i = lexer->index - 10; i < lexer->index; ++i) {
        putc(lexer->src[i], stdout);
    }
    exit(1);

}


void lexer_update(struct Lexer * lexer, unsigned int increment) {
    const unsigned int offset = lexer->index + increment;
    while (lexer->index < offset) {
        lexer_skip_whitespace(lexer);
        lexer_advance(lexer);
    }
        
}


void lexer_skip_whitespace(struct Lexer * lexer) {

    while (1) {
        switch (lexer->c) {
            case ' ':
            case '\t':
                break;
            case '\r':
                lexer->pos = 0; break;
            default:
                return;
        }
        lexer_advance(lexer);
    }

}


char lexer_peek(struct Lexer * lexer, unsigned int offset) {
    
    const unsigned int index = lexer->index + offset;
    if (index < lexer->size)
        return lexer->src[index];

    return 0;
}


struct Token * lexer_parse_id(struct Lexer * lexer) {
    const unsigned int _start = lexer->pos;
    unsigned int start_index = lexer->index, length;
    char * id;
    
    char peek = lexer_peek(lexer, 1);
    while (isalpha(peek) || peek == '_' || isdigit(peek)) {
        lexer_advance(lexer);
        peek = lexer_peek(lexer, 1);
    }

    length = lexer->index - start_index + 1;
    id = malloc(sizeof(char) * (length + 1));

    memcpy(id, lexer->src + start_index, length);
    id[length] = 0;

    if (str_to_operator(id, OP_TYPE_ANY).key != OP_NOT_FOUND)
        return init_token(id, length + 1, TOKEN_OP, lexer->line, _start);

    return init_token(id, length + 1, TOKEN_ID, lexer->line, _start);
}


struct Token * lexer_parse_string_literal(struct Lexer * lexer) {
    const unsigned int _start = lexer->pos;
	int start = lexer->index + 1, end = start;

	while (lexer->src[++end] != lexer->c) {
		if(end == lexer->size) {
			println("\nEnd of file found while reading string.");
			exit(1);
		}
		else if (lexer->src[end] == '\n') {
			println("[Parsing Error]: {u}:{u} Newline inside of string literal" 
						"is not allowed\n", lexer->line, lexer->pos);
			exit(1);
		}
	}

	const size_t length = end - start;
	char * string = malloc(sizeof(char) * (length + 1));
	memcpy(string, lexer->src + start, length);
	string[length] = 0;

    lexer_update(lexer, length + 1); // add one for the extra " at the end

	return init_token(string, length + 1, TOKEN_STRING_LITERAL, lexer->line, _start);

}


struct Token * lexer_parse_int(struct Lexer * lexer) {
    unsigned int _start = lexer->pos;

	char * number_string = malloc(0);
    unsigned int length = 0;
    char peek = lexer->c;

	while (isdigit(peek)) {
        number_string = realloc(number_string, sizeof(char) * (length + 2));
        number_string[length] = peek;
		
        peek = lexer_peek(lexer, ++length);
        while (peek == '_')
			peek = lexer_peek(lexer, ++length);
	}
    number_string[length] = 0;

    lexer_update(lexer, length - 1);

	return init_token(number_string, length + 1, TOKEN_INT, lexer->line, _start);
}


struct Token * lexer_parse_multi_line_comment(struct Lexer * lexer) {
    while(lexer->c != EOF) {
        if (lexer->c == '*' && lexer_peek(lexer, 1) == '/')
            break;
        else if (lexer->c == '\n') {
            lexer->pos = 0;
            lexer->line += 1;
        }
        lexer_advance(lexer);
    }

	return lexer_next_token(lexer);
}


struct Token * lexer_parse_single_line_comment(struct Lexer * lexer) {
    unsigned int offset = 0;

    while(lexer_peek(lexer, ++offset) != '\n' 
            && lexer_peek(lexer, offset) != EOF);

    lexer_update(lexer, offset);

    return lexer_next_token(lexer);
}

struct Token * lexer_parse_operator(struct Lexer * lexer) {
    unsigned int _start = lexer->pos;
    size_t length = 0;
    char * str = malloc(0);
op_loop:
    switch (lexer->c) {
        case '/':
        case '+':
        case '-':
        case '=':
        case '<':
        case '>':
        case '*':
        case '^':
        case '&':
        case '~':
        case '.':
        case '%':
        case '!':
        case '?':
        case '|':
            str = realloc(str, (length + 2) * sizeof(char));
            str[length++] = lexer->c;
            lexer_advance(lexer);
            goto op_loop;
        default:
            break;
    }
    str[length] = 0;
    return init_token(str, length + 1, TOKEN_OP, lexer->line, _start);
}

struct Token * lexer_advance_with(struct Lexer * lexer, struct Token * token) {
    lexer_advance(lexer);
    return token;
}


struct Token * lexer_advance_current(struct Lexer * lexer, enum token_t type) {
    return lexer_advance_with(lexer, init_token(NULL, 0, type, lexer->line, lexer->pos));
}


struct Token * lexer_next_token(struct Lexer * lexer) {
    
    struct Token * token, * next;
    char peek;
    lexer_skip_whitespace(lexer);

    switch (lexer->c) {
        case (0):
            break;
        case '\n':
            lexer->pos = 0;
            lexer->line += 1;
            return lexer_advance_current(lexer, TOKEN_LINE_BREAK);
        case '(':
            return lexer_advance_current(lexer, TOKEN_LPAREN);
        case ')':
            return lexer_advance_current(lexer, TOKEN_RPAREN);
        case '{':
            return lexer_advance_current(lexer, TOKEN_LBRACE);
        case '}':
            return lexer_advance_current(lexer, TOKEN_RBRACE);
        case '[':
            return lexer_advance_current(lexer, TOKEN_LBRACKET);
        case ']':
            return lexer_advance_current(lexer, TOKEN_RBRACKET);
        case ',':
            return lexer_advance_current(lexer, TOKEN_COMMA);
        case ':':
            return lexer_advance_current(lexer, TOKEN_COLON);
        case ';':
            return lexer_advance_current(lexer, TOKEN_SEMI);
        case '"':
            return lexer_advance_with(lexer, lexer_parse_string_literal(lexer));
        case '\'':
            return lexer_advance_current(lexer, TOKEN_SINGLE_QUOTE);
        case '/':
            peek = lexer_peek(lexer, 1);
            
            if (peek == '/')
                return lexer_advance_with(lexer, lexer_parse_single_line_comment(lexer));
            else if (peek == '*')
                return lexer_advance_with(lexer, lexer_parse_multi_line_comment(lexer));
        case '+':
        case '-':
        case '=':
        case '<':
        case '>':
        case '*':
        case '^':
        case '&':
        case '~':
        case '.':
        case '%':
        case '!':
        case '?':
        case '|':
            return lexer_parse_operator(lexer);
        default:
            if (isalpha(lexer->c) || lexer->c == '_') {
                return lexer_advance_with(lexer, lexer_parse_id(lexer));
            }

            if (isdigit(lexer->c)) {
                return lexer_advance_with(lexer, lexer_parse_int(lexer));
            }

            println("\n[Lexer]: Unexpected characther: {c}({u})", lexer->c, lexer->c);
            exit(1);

    }

    return init_token(NULL, 0, TOKEN_EOF, lexer->line, lexer->pos);
}
