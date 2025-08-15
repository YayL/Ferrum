#include "parser/lexer.h"

#include "parser/operators.h"

#include <ctype.h>

struct Lexer * init_lexer(char * src, size_t length) {

    struct Lexer * lexer = malloc(sizeof(struct Lexer));

    lexer->src = src;
    lexer->size = length;
    lexer->c = src[0];
    lexer->pos = lexer->line = 1;
    lexer->index = 0;
    lexer->tok = init_token();

    return lexer;
}


void lexer_advance(struct Lexer * lexer) {
    if (lexer->c == EOF || lexer->size <= lexer->index) {
        println("[Lexer]: End of file found while lexing: ");
        println("Index = {u}, Size = {u}, C = {c}({i})", lexer->index, lexer->size, lexer->c, lexer->c);
        for (unsigned int i = lexer->index - 10; i < lexer->index; ++i) {
            putc(lexer->src[i], stdout);
        }
        exit(1);
    }

    lexer->c = lexer->src[++lexer->index];
    lexer->pos++;
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

char lexer_is_operator_char(char c) {
    switch (c) {
        case '/':
        case '+':
        case '-':
        case '=':
        case '<':
        case '>':
        case '(':
        case ')':
        case '[':
        case ']':
        case ':':
        case '*':
        case '^':
        case '&':
        case '~':
        case '.':
        case '%':
        case '!':
        case '?':
        case '|':
            return 1;
    }
    
    return 0;
}


void lexer_parse_id(struct Lexer * lexer) {
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
    
    set_token(lexer->interner, lexer->tok, id, length + 1, is_operator(id) ? TOKEN_OP : TOKEN_ID, lexer->line, _start);
}


void lexer_parse_string_literal(struct Lexer * lexer) {
    const unsigned int _start = lexer->pos;
	int start = lexer->index + 1, end = start;

	while (lexer->src[end] != lexer->c) {
		if(end == lexer->size) {
			println("\nEnd of file found while reading string.");
			exit(1);
		}
		else if (lexer->src[end] == '\n') {
			println("[Parsing Error]: {u}:{u} Newline inside of string literal" 
						"is not allowed\n", lexer->line, lexer->pos);
			exit(1);
		}
        end += 1;
	}

	const size_t length = end - start;
	char * string = malloc(sizeof(char) * (length + 1));
	memcpy(string, lexer->src + start, length);
	string[length] = 0;

    lexer_update(lexer, length + 1); // add one for the extra " at the end

	set_token(lexer->interner, lexer->tok, string, length + 1, TOKEN_STRING_LITERAL, lexer->line, _start);

}


void lexer_parse_int(struct Lexer * lexer) {
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

	set_token(lexer->interner, lexer->tok, number_string, length + 1, TOKEN_INT, lexer->line, _start);
}

void lexer_parse_multi_line_comment(struct Lexer * lexer) {
    struct Token token = *lexer->tok;
    char prev = 0;

    while(lexer->c != EOF && !(prev == '*' && lexer->c == '/')) {
        if (lexer->c == '\n') {
            lexer->pos = 0;
            lexer->line += 1;
        } else if (lexer->c == '\0') {
            FATAL("Unclosed multiline comment at: {2i::}", token.line, token.pos);
        }
        prev = lexer->c;
        lexer_advance(lexer);
    }
    lexer_advance(lexer);
}


void lexer_parse_single_line_comment(struct Lexer * lexer) {
    unsigned int offset = 0;    
    char c;
    
    while(lexer->c != '\n' && lexer->c != EOF) 
        lexer_advance(lexer);

    lexer_advance(lexer);
}

void lexer_parse_operator(struct Lexer * lexer) {
    struct Operator op;

    // arr is an buf keeping track of the indeces of all possible operators matching the string so far and that are longer than current matching
    int arr[sizeof(op_conversion) / sizeof(op_conversion[0])] = {0}, arr_index = 0, arr_size;
    size_t offset = 1, length = 0;
    char c = lexer->src[lexer->index - 1];

    // the 2 loops below are just searching for what operator is written and trying to find it effectively

    // check first characther and set length to 1 if found a complete answer or add to arr if a possible match 
    for (int i = 0; i < sizeof(op_conversion) / sizeof(op_conversion[0]); ++i) {
        if (c == op_conversion[i].str[0]) {
            // if operator is longer than one char
            if (op_conversion[i].str[1])
                arr[arr_index++] = i;
            else
                length = 1;
        // is enclosed operator and the ending of enclosed matches char
        } else if (op_conversion[i].enclosed && c == op_conversion[i].str[op_conversion[i].enclosed_offset]) {
            if (op_conversion[i].str[op_conversion[i].enclosed_offset + 1])
                arr[arr_index++] = i;
            else
                length = 1;
        }
    }
    
    c = lexer->c;
    while (lexer_is_operator_char(c)) {
        arr_size = arr_index; // number of possible results
        arr_index = 0;
        // same as previous for loop but keeps track of an offset for the length of the current operator matching
        for (int i = 0; i < arr_size; ++i) {
            op = op_conversion[arr[i]];
            if (c == op.str[offset]) {
                if (op.str[offset + 1])
                    arr[arr_index++] = arr[i];
                else
                    length = offset + 1;
            } else if (op.enclosed && c == op.str[offset + op.enclosed_offset]) {
                if (op.str[op.enclosed_offset + offset + 1])
                    arr[arr_index++] = arr[i];
                else
                    length = offset + 1;
            }
        }
        c = lexer->src[lexer->index + offset++];
    }

    // if length is 0 then there was no match as length is the length of an existing operator
    if (length == 0) {
        lexer->src[lexer->index + offset] = '\0';
        FATAL("Invalid operator '{s}'", &lexer->src[lexer->index - 1]);
    }
    
    char * str = malloc(sizeof(char) * (length + 1));
    strncpy(str, &lexer->src[lexer->index - 1], length);
    str[length] = '\0';

    // lexer_update updates lexer->pos so must keep track of that value for set_token
    offset = lexer->pos;

    lexer_update(lexer, length - 1);
    set_token(lexer->interner, lexer->tok, str, length, TOKEN_OP, lexer->line, offset);
}

void lexer_advance_current(struct Lexer * lexer, enum token_t type) {
    lexer_advance(lexer);
    set_token(lexer->interner, lexer->tok, NULL, 0, type, lexer->line, lexer->pos);
}


void lexer_next_token(struct Lexer * lexer) { 
    char peek;
    lexer_skip_whitespace(lexer);
    
    switch (lexer->c) {
        case (0):
            set_token(lexer->interner, lexer->tok, NULL, 0, TOKEN_EOF, lexer->line, lexer->pos);
            break;
        case '\n':
            lexer->pos = 0;
            lexer->line += 1;
            lexer_advance_current(lexer, TOKEN_LINE_BREAK);
            break;
        case '{':
            lexer_advance_current(lexer, TOKEN_LBRACE);
            break;
        case '}':
            lexer_advance_current(lexer, TOKEN_RBRACE);
            break;
        case ',':
            return lexer_advance_current(lexer, TOKEN_COMMA);
        case ';':
            lexer_advance_current(lexer, TOKEN_SEMI);
            break;
        case '\'':
        case '"':
            lexer_parse_string_literal(lexer);
            lexer_advance(lexer);
            break;
        case '\\':
            return lexer_advance_current(lexer, TOKEN_BACKSLASH);
        case '/':
            peek = lexer_peek(lexer, 1);
            if (peek == '/') {
                lexer_parse_single_line_comment(lexer);
                lexer_next_token(lexer);
                break;
            } else if (peek == '*') {
                lexer_parse_multi_line_comment(lexer);
                lexer_next_token(lexer);
                break;
            }

            return lexer_advance_current(lexer, TOKEN_SLASH);
        case '-':
            return lexer_advance_current(lexer, TOKEN_MINUS);
        case '=':
            return lexer_advance_current(lexer, TOKEN_EQUAL);
        case '<':
            return lexer_advance_current(lexer, TOKEN_LT);
        case '>':
            return lexer_advance_current(lexer, TOKEN_GT);
        case '(':
            return lexer_advance_current(lexer, TOKEN_LPAREN);
        case '_':
            return lexer_advance_current(lexer, TOKEN_UNDERSCORE);
        case ')':
            return lexer_advance_current(lexer, TOKEN_RPAREN);
        case '[':
            return lexer_advance_current(lexer, TOKEN_LBRACKET);
        case ']':
            return lexer_advance_current(lexer, TOKEN_RBRACKET);
        case ':':
            return lexer_advance_current(lexer, TOKEN_COLON);
        case '*':
            return lexer_advance_current(lexer, TOKEN_ASTERISK);
        case '^':
            return lexer_advance_current(lexer, TOKEN_CARET);
        case '&':
            return lexer_advance_current(lexer, TOKEN_AMPERSAND);
        case '~':
            return lexer_advance_current(lexer, TOKEN_TILDA);
        case '.':
            return lexer_advance_current(lexer, TOKEN_DOT);
        case '%':
            return lexer_advance_current(lexer, TOKEN_PERCENT);
        case '!':
            return lexer_advance_current(lexer, TOKEN_EXCLAMATION_MARK);
        case '?':
            return lexer_advance_current(lexer, TOKEN_QUESTION_MARK);
        case '|':
            return lexer_advance_current(lexer, TOKEN_VERTICAL_LINE);
        case '+':
            return lexer_advance_current(lexer, TOKEN_PLUS);
        default:
            if (isalpha(lexer->c) || lexer->c == '#') {
                lexer_parse_id(lexer);
                lexer_advance(lexer);
                break;
            }

            if (isdigit(lexer->c)) {
                lexer_parse_int(lexer);
                lexer_advance(lexer);
                break;
            }

            println("\n[Lexer]: Unexpected characther: {c}({u})", lexer->c, lexer->c);
            exit(1);

    }
}
