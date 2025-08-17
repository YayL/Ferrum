#pragma once

#include "common/common.h"
#include "parser/token.h"
#include "codegen/AST.h"
#include "tables/interner.h"

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

enum OP_enclosed {
    NORMAL,
    ENCLOSED, // an operator where the last operand is enclosed by certain characthers 
              // separated by a \0 to separate the starting characthers to the ending
};

#define OPERATORS_LIST(f) \
    f(OP_NOT_FOUND, UNARY_PRE, 0, LEFT, NORMAL, 0, "") \
    f(PARENTHESES, UNARY_PRE, 0, LEFT, ENCLOSED, 2, "(\0)") \
    \
    f(PRE_INCREMENT, UNARY_PRE, 1, LEFT, NORMAL, 0, "++") \
    f(POST_INCREMENT, UNARY_POST, 1, LEFT, NORMAL, 0, "++") \
    f(PRE_DECREMENT, UNARY_PRE, 1, LEFT, NORMAL, 0, "--") \
    f(POST_DECREMENT, UNARY_POST, 1, LEFT, NORMAL, 0, "--") \
    \
    f(MEMBER_ACCESS, BINARY, 1, LEFT, NORMAL, 0, ".") \
    f(CALL, BINARY, 1, LEFT, ENCLOSED, 2, "(\0)") \
    f(SUBSCRIPT, BINARY, 1, LEFT, ENCLOSED, 2, "[\0]") \
    \
    f(UNARY_PLUS, UNARY_PRE, 2, RIGHT, NORMAL, 0, "+") \
    f(UNARY_MINUS, UNARY_PRE, 2, RIGHT, NORMAL, 0, "-") \
    f(LOGICAL_NOT, UNARY_PRE, 2, RIGHT, NORMAL, 0, "!") \
    f(BITWISE_NOT, UNARY_PRE, 2, RIGHT, NORMAL, 0, "~") \
    f(DEREFERENCE, UNARY_PRE, 2, RIGHT, NORMAL, 0, "*") \
    f(ADDRESS_OF, UNARY_PRE, 2, RIGHT, NORMAL, 0, "&") \
    \
    f(MULTIPLICATION, BINARY, 3, LEFT, NORMAL, 0, "*") \
    f(DIVISION, BINARY, 3, LEFT, NORMAL, 0, "/") \
    f(REMAINDER, BINARY, 3, LEFT, NORMAL, 0, "%") \
    \
    f(ADDITION, BINARY, 4, LEFT, NORMAL, 0, "+") \
    f(SUBTRACTION, BINARY, 4, LEFT, NORMAL, 0, "-") \
    \
    f(BITWISE_LEFT_SHIFT, BINARY, 5, LEFT, NORMAL, 0, "<<") \
    f(BITWISE_RIGHT_SHIFT, BINARY, 5, LEFT, NORMAL, 0, ">>") \
    \
    f(LESS_THAN, BINARY, 6, LEFT, NORMAL, 0, "<") \
    f(LESS_EQUAL_TO, BINARY, 6, LEFT, NORMAL, 0, "<=") \
    f(GREATER_THAN, BINARY, 6, LEFT, NORMAL, 0, ">") \
    f(GREATER_EQUAL_TO, BINARY, 6, LEFT, NORMAL, 0, ">=") \
    \
    f(EQUAL, BINARY, 7, LEFT, NORMAL, 0, "==") \
    f(NOT_EQUAL, BINARY, 7, LEFT, NORMAL, 0, "!=") \
    \
    f(BITWISE_AND, BINARY, 8, LEFT, NORMAL, 0, "&") \
    f(BITWISE_XOR, BINARY, 9, LEFT, NORMAL, 0, "^") \
    f(BITWISE_OR, BINARY, 10, LEFT, NORMAL, 0, "|") \
    \
    f(LOGICAL_AND, BINARY, 11, LEFT, NORMAL, 0, "&&") \
    f(LOGICAL_OR, BINARY, 12, LEFT, NORMAL, 0, "||") \
    \
    f(CAST, UNARY_POST, 13, LEFT, NORMAL, 0, "to") \
    f(BIT_CAST, UNARY_POST, 13, LEFT, NORMAL, 0, "as") \
    \
    f(TERNARY, BINARY, 14, RIGHT, NORMAL, 0, "?") \
    f(TERNARY_BODY, BINARY, 14, RIGHT, NORMAL, 0, ":") \
    \
    f(ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "=") \
    f(ADD_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "+=") \
    f(SUBTRACT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "-=") \
    f(PRODUCT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "*=") \
    f(QUOTIENT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "/=") \
    f(REMAINDER_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "%=") \
    f(BITWISE_LEFT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "<<=") \
    f(BITWISE_RIGHT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, ">>=") \
    f(BITWISE_AND_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "&=") \
    f(BITWISE_XOR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "^=") \
    f(BITWISE_OR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "|=")

#define OPERATORS_ENUM_EL(ENUM, ...) ENUM,

enum Operators {
    OPERATORS_LIST(OPERATORS_ENUM_EL)
};

#define OPERATOR_CONVERSION_EL(KEY, MODE, PRECEDENCE, ASSOCIATIVITY, ENCLOSED, ENCLOSED_OFF, REPR) \
    {REPR, INVALID_INTERN_ID, KEY, MODE, ASSOCIATIVITY, ENCLOSED, ENCLOSED_OFF, PRECEDENCE},

static struct Operator {
    char * str;
    unsigned int intern_id;
    enum Operators key;
    enum OP_mode mode;
    enum OP_associativity associativity;
    enum OP_enclosed enclosed;
    char enclosed_offset;
    char precedence;
} op_conversion [] = {
    OPERATORS_LIST(OPERATOR_CONVERSION_EL)
};

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag);
char is_operator(const char * str);

void operators_intern();

unsigned int operator_get_intern_id(enum Operators op);
const char * operator_get_runtime_name(enum Operators op);
char * operator_to_str(struct Operator * operator);
