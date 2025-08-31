#include "parser/operators.h"

#include "tables/interner.h"
#include "parser/lexer.h"

#define OPERATOR_CONVERSION_EL(KEY, MODE, PRECEDENCE, ASSOCIATIVITY, ENCLOSED, REPR) \
    { .str = REPR, .intern_id = INVALID_ID, .key = KEY, .mode = MODE, .associativity = ASSOCIATIVITY, .enclosed = ENCLOSED, .enclosed_offset = (sizeof(REPR) / sizeof(char)) / 2, .precedence = PRECEDENCE},
struct Operator op_list[] = {
    OPERATORS_LIST(OPERATOR_CONVERSION_EL)
};

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

void lexer_parse_operator(struct Lexer * lexer) {
    // arr is an buf keeping track of the indeces of all possible operators matching the string so far and that are longer than current matching
    int arr[sizeof(op_list) / sizeof(op_list[0])] = {0}, arr_index = 0, arr_size;
    size_t offset = 1, length = 0;
    char c = lexer->src[lexer->index - 1];

    // the 2 loops below are just searching for what operator is written and trying to find it effectively

    // check first characther and set length to 1 if found a complete answer or add to arr if a possible match 
    for (int i = 0; i < sizeof(op_list) / sizeof(op_list[0]); ++i) {
        struct Operator op = op_list[i];
        if (c == op.str[0]) {
            // if operator is longer than one char
            if (op.str[1])
                arr[arr_index++] = i;
            else
                length = 1;
        // is enclosed operator and the ending of enclosed matches char
        } else if (op.enclosed && c == op.str[op.enclosed_offset]) {
            if (op.str[op.enclosed_offset + 1])
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
            struct Operator op = op_list[arr[i]];
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
    set_token(&lexer->tok, str, length, TOKEN_OP, lexer->line, offset);
}

unsigned int operator_get_count() {
    return sizeof(op_list) / sizeof(op_list[0]);
}

struct Operator operator_get(enum Operators operator) {
    ASSERT1(operator < operator_get_count());
    ASSERT1(op_list[operator].key == operator);
    return op_list[operator];
}

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag) {
    const char * op_str;
    for (int i = 0; i < sizeof(op_list) / sizeof(op_list[0]); ++i) {
        op_str = op_list[i].str;

        if (op_list[i].enclosed == ENCLOSED && op_list[i].mode == mode) {
            if (!strcmp(str, op_str)) {
                if (enclosed_flag != NULL)
                    *enclosed_flag = 0;
                return op_list[i];
            } else if (!strcmp(str, op_str + strlen(op_str) + 1)) {
                if (enclosed_flag != NULL)
                    *enclosed_flag = 1;
                return op_list[i];
            }
        }

        if ((mode == OP_TYPE_ANY || op_list[i].mode == mode) && !strcmp(str, op_str))
            return op_list[i];
    }
    return op_list[0];
}

char is_operator(const char * str) {
    for (int i = 0; i < sizeof(op_list) / sizeof(op_list[0]); ++i) {
        if (!strcmp(str, op_list[i].str)) {
            return op_list[i].key != OP_NOT_FOUND;
        } else if (op_list[i].enclosed && !strcmp(str, op_list[i].str + strlen(op_list[i].str) + 1)) {
            return 1;
        }
    }

    return 0;
}

#define OPERATOR_GET_RUNTIME_NAME(KEY) "#OP_" #KEY
#define OPERATOR_GET(KEY) op_list[KEY]
#define OPERATOR_INTERN(KEY, ...) \
    OPERATOR_GET(KEY).intern_id = interner_intern(STRING_FROM_LITERAL(OPERATOR_GET_RUNTIME_NAME(KEY)));

void operators_intern() {
    OPERATORS_LIST(OPERATOR_INTERN)
}

ID operator_get_intern_id(enum Operators op) {
    return OPERATOR_GET(op).intern_id;
}

#define OPERATOR_RUNTIME_NAME(KEY, ...) \
    case KEY: return OPERATOR_GET_RUNTIME_NAME(KEY);

const char * operator_get_runtime_name(enum Operators op) {
    switch (op) {
        OPERATORS_LIST(OPERATOR_RUNTIME_NAME)
    }
}

char * operator_to_str(struct Operator * operator) {
    return format("<name='{s}', op='{s}', precedence='{u}', mode='{s}', enclosed='{b}', associativity='{s}'>",
                  operator_get_runtime_name(operator->key),
                  operator->str,
                  operator->precedence,
                  operator->mode == BINARY ? "binary" : "unary",
                  operator->enclosed == ENCLOSED,
                  operator->associativity == LEFT ? "left" : "right"
                  );
}
