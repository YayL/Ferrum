#pragma once

#include "common/ID.h"
#include "common/data/sourcespan.h"

enum OP_mode {
    UNARY_PRE, // an operator taking one operand preceeding itself: ++a
    UNARY_POST,// an operator taking one operand proceeding itself: a++
    BINARY, // an operator taking two operands on either side of the operator
    OP_TYPE_ANY
};

enum OP_associativity {
    LEFT,
    RIGHT
};

enum OP_speciality {
    NORMAL,
    ALPHABETIC,
    ENCLOSED, // an operator where the last operand is enclosed by certain characthers 
              // separated by a \0 to separate the starting characthers to the ending
};

#define OPERATORS_LIST(f) \
    f(OP_NOT_FOUND, UNARY_PRE, 0, LEFT, NORMAL, "") \
    f(PARENTHESES, UNARY_PRE, 0, LEFT, ENCLOSED, "(\0)") \
    \
    f(MEMBER_ACCESS, BINARY, 1, LEFT, NORMAL, ".") \
    f(CALL, BINARY, 1, LEFT, ENCLOSED, "(\0)") \
    f(TEMPLATE, UNARY_POST, 1, LEFT, ENCLOSED, "<\0>") \
    f(SUBSCRIPT, BINARY, 1, LEFT, ENCLOSED, "[\0]") \
    \
    f(PRE_INCREMENT, UNARY_PRE, 1, LEFT, NORMAL, "++") \
    f(POST_INCREMENT, UNARY_POST, 1, LEFT, NORMAL, "++") \
    f(PRE_DECREMENT, UNARY_PRE, 1, LEFT, NORMAL, "--") \
    f(POST_DECREMENT, UNARY_POST, 1, LEFT, NORMAL, "--") \
    \
    f(CAST, UNARY_POST, 3, LEFT, ALPHABETIC, "to") \
    \
    f(UNARY_PLUS, UNARY_PRE, 4, RIGHT, NORMAL, "+") \
    f(UNARY_MINUS, UNARY_PRE, 4, RIGHT, NORMAL, "-") \
    f(LOGICAL_NOT, UNARY_PRE, 4, RIGHT, NORMAL, "!") \
    f(BITWISE_NOT, UNARY_PRE, 4, RIGHT, NORMAL, "~") \
    f(DEREFERENCE, UNARY_PRE, 4, RIGHT, NORMAL, "*") \
    f(ADDRESS_OF, UNARY_PRE, 4, RIGHT, NORMAL, "&") \
    f(MUT_ADDRESS_OF, UNARY_PRE, 4, RIGHT, NORMAL, "&mut") \
    \
    f(MULTIPLICATION, BINARY, 5, LEFT, NORMAL, "*") \
    f(DIVISION, BINARY, 5, LEFT, NORMAL, "/") \
    f(REMAINDER, BINARY, 5, LEFT, NORMAL, "%") \
    \
    f(ADDITION, BINARY, 6, LEFT, NORMAL, "+") \
    f(SUBTRACTION, BINARY, 6, LEFT, NORMAL, "-") \
    \
    f(BITWISE_LEFT_SHIFT, BINARY, 7, LEFT, NORMAL, "<<") \
    f(BITWISE_RIGHT_SHIFT, BINARY, 7, LEFT, NORMAL, ">>") \
    \
    f(LESS_THAN, BINARY, 8, LEFT, NORMAL, "<") \
    f(LESS_EQUAL_TO, BINARY, 8, LEFT, NORMAL, "<=") \
    f(GREATER_THAN, BINARY, 8, LEFT, NORMAL, ">") \
    f(GREATER_EQUAL_TO, BINARY, 8, LEFT, NORMAL, ">=") \
    \
    f(EQUAL, BINARY, 9, LEFT, NORMAL, "==") \
    f(NOT_EQUAL, BINARY, 9, LEFT, NORMAL, "!=") \
    \
    f(BITWISE_AND, BINARY, 10, LEFT, NORMAL, "&") \
    f(BITWISE_XOR, BINARY, 11, LEFT, NORMAL, "^") \
    f(BITWISE_OR, BINARY, 12, LEFT, NORMAL, "|") \
    \
    f(LOGICAL_AND, BINARY, 13, LEFT, NORMAL, "&&") \
    f(LOGICAL_OR, BINARY, 14, LEFT, NORMAL, "||") \
    \
    f(TERNARY, BINARY, 15, RIGHT, NORMAL, "?") \
    f(TERNARY_BODY, BINARY, 15, RIGHT, NORMAL, ":") \
    \
    f(ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "=") \
    f(ADD_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "+=") \
    f(SUBTRACT_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "-=") \
    f(PRODUCT_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "*=") \
    f(QUOTIENT_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "/=") \
    f(REMAINDER_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "%=") \
    f(BITWISE_LEFT_SHIFT_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "<<=") \
    f(BITWISE_RIGHT_SHIFT_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, ">>=") \
    f(BITWISE_AND_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "&=") \
    f(BITWISE_XOR_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "^=") \
    f(BITWISE_OR_ASSIGNMENT, BINARY, 16, RIGHT, NORMAL, "|=")

#define OPERATORS_ENUM_EL(ENUM, ...) ENUM,

enum Operators {
    OPERATORS_LIST(OPERATORS_ENUM_EL)
};

typedef struct Operator {
    const char * str;
    ID intern_id;
    enum Operators key;
    enum OP_mode mode;
    enum OP_associativity associativity;
    enum OP_speciality enclosed;
    char enclosed_offset;
    char precedence;
} Operator;

void operators_intern();

struct Operator str_to_operator(const SourceSpan str, enum OP_mode mode, char * enclosed_flag);
char id_is_operator(const SourceSpan span);

struct Operator operator_get(enum Operators operator);
unsigned int operator_get_count();
ID operator_get_intern_id(enum Operators op);
const char * operator_get_runtime_name(enum Operators op);
char * operator_to_str(struct Operator operator);
