#include "parser/lexer.h"

#include "parser/operators.h"
#include "common/io.h"

#include <ctype.h>

struct Lexer lexer_init(const char * file_path) {
    size_t length;
    char * src = read_file(file_path, &length);

    struct Lexer lexer = {
        .src = src,
        .size = length,
        .c = src[0],
        .pos = 1,
        .line = 1,
        .index = 0,
        .tok = init_token()
    };

    lexer_next_token(&lexer);

    return lexer;
}

void lexer_free(struct Lexer * lexer) {
    free(lexer->src);
}

void lexer_update_token(struct Lexer * lexer, const char * start, size_t length, enum token_t type) {
    lexer->tok.span = source_span_init(start, length);
    lexer->tok.interner_id = INVALID_ID;

    if (type == TOKEN_ID) {
        lexer->tok.interner_id = interner_intern(lexer->tok.span);
    }

    lexer->tok.type = type;
    lexer->tok.line = lexer->line;
    lexer->tok.pos = lexer->pos;
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

void lexer_parse_id(struct Lexer * lexer) {
    unsigned int length = 0;
    
    char peek;
    while (peek = lexer_peek(lexer, length++), isalpha(peek) || peek == '_' || (length > 1 && isdigit(peek)) || (length == 1 && peek == '#'));
    length -= 1; // Went pass the end of ID

    SourceSpan id_span = source_span_init(lexer->src + lexer->index, length);

    lexer_update_token(lexer, lexer->src + lexer->index, length, id_is_operator(id_span) ? TOKEN_OP : TOKEN_ID);
    lexer_update(lexer, length);
}


void lexer_parse_string_literal(struct Lexer * lexer) {
	unsigned int length = 0;
    char peek;

    ASSERT1(lexer->c != '\'');

    // Skip first '"' so perform pre-increment
	while (peek = lexer_peek(lexer, ++length), isprint(peek) && peek != '"');

    lexer_update_token(lexer, lexer->src + lexer->index + 1, length - 1, TOKEN_STRING_LITERAL);
    lexer_update(lexer, length + 1); // Add 1 to skip last '"'
}


void lexer_parse_int(struct Lexer * lexer) {
    unsigned int length = 0;
    char peek;

	while (peek = lexer_peek(lexer, length++), isdigit(peek) || peek == '_');
    length -= 1; // Went pass the end of int

    lexer_update_token(lexer, lexer->src + lexer->index, length, TOKEN_INT);
    lexer_update(lexer, length);
}

void lexer_parse_multi_line_comment(struct Lexer * lexer) {
    struct Token token = lexer->tok;
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

    lexer_advance(lexer); // Skip last '/'
    lexer_next_token(lexer);
}


void lexer_parse_single_line_comment(struct Lexer * lexer) {
    unsigned int offset = 0;    
    char c;
    
    while(lexer->c != '\n' && lexer->c != EOF) {
        lexer_advance(lexer);
    }

    lexer_next_token(lexer); // Keep linebreak
}


void lexer_advance_current(struct Lexer * lexer, enum token_t type) {
    lexer_advance(lexer);
    lexer_update_token(lexer, NULL, 0, type);
}


void lexer_next_token(struct Lexer * lexer) { 
    char peek;
    lexer_skip_whitespace(lexer);
    
    switch (lexer->c) {
        case (0):
            lexer_update_token(lexer, NULL, 0, TOKEN_EOF);
            // set_token(&lexer->tok, NULL, 0, TOKEN_EOF, lexer->line, lexer->pos);
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
            break;
        case '\\':
            return lexer_advance_current(lexer, TOKEN_BACKSLASH);
        case '/':
            peek = lexer_peek(lexer, 1);
            if (peek == '/') {
                lexer_parse_single_line_comment(lexer);
                break;
            } else if (peek == '*') {
                lexer_parse_multi_line_comment(lexer);
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
                break;
            }

            if (isdigit(lexer->c)) {
                lexer_parse_int(lexer);
                break;
            }

            println("\n[Lexer]: Unexpected characther: {c}({u})", lexer->c, lexer->c);
            exit(1);

    }
}
