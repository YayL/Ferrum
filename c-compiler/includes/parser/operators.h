#pragma once

#include "common/common.h"
#include "codegen/AST.h"

enum Operators {
    OP_NOT_FOUND,
    PARENTHESES,
    BRACKETS,
    ARROW,

    INCREMENT,
    DECREMENT,
    MEMBER_ACCESS,
    MEMBER_ACCESS_PTR,
    CALL,
    SUBSCRIPT,

    UNARY_PLUS,
    UNARY_MINUS,
    LOGICAL_NOT,
    BITWISE_NOT,
    DEREFERENCE,
    ADDRESS_OF,
    CAST,
    BIT_CAST,

    MULTIPLICATION,
    DIVISION,
    REMAINDER,

    ADDITION,
    SUBTRACTION,

    BITWISE_LEFT_SHIFT,
    BITWISE_RIGHT_SHIFT,

    LESS_THAN,
    LESS_EQUAL_TO,
    GREATER_THAN,
    GREATER_EQUAL_TO,

    EQUAL,
    NOT_EQUAL,

    BITWISE_AND,
    BITWISE_XOR,
    BITWISE_OR,

    LOGICAL_AND,
    LOGICAL_OR,

    TERNARY,
    TERNARY_BODY,

    ASSIGNMENT,
    ADD_ASSIGNMENT,
    SUBTRACT_ASSIGNMENT,
    PRODUCT_ASSIGNMENT,
    QUOTIENT_ASSIGNMENT,
    REMAINDER_ASSIGNMENT,
    BITWISE_LEFT_SHIFT_ASSIGNMENT,
    BITWISE_RIGHT_SHIFT_ASSIGNMENT,
    BITWISE_AND_ASSIGNMENT,
    BITWISE_XOR_ASSIGNMENT,
    BITWISE_OR_ASSIGNMENT,
};

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

const static struct Operator {
    enum Operators key;
    enum OP_mode mode;
    char precedence;
    enum OP_associativity associativity;
    enum OP_enclosed enclosed;
    char enclosed_offset;
    char * str;
} op_conversion [] = {
    {OP_NOT_FOUND, UNARY_PRE, 0, LEFT, NORMAL, 0, ""},
    {PARENTHESES, UNARY_PRE, 0, LEFT, ENCLOSED, 2, "(\0)"},
    //{BRACKETS, UNARY_PRE, 0, LEFT, ENCLOSED, 0, "[\0]"}, ARRAY DEFINITION
    
    {INCREMENT, UNARY_PRE, 1, LEFT, NORMAL, 0, "++"},
    {INCREMENT, UNARY_POST, 1, LEFT, NORMAL, 0, "++"},
    {DECREMENT, UNARY_PRE, 1, LEFT, NORMAL, 0, "--"},
    {DECREMENT, UNARY_POST, 1, LEFT, NORMAL, 0, "--"},

    {MEMBER_ACCESS, BINARY, 1, LEFT, NORMAL, 0, "."},
    {ARROW, BINARY, 1, LEFT, NORMAL, 0, "->"},
    {CALL, BINARY, 1, LEFT, ENCLOSED, 2, "(\0)"},
    {SUBSCRIPT, BINARY, 1, LEFT, ENCLOSED, 2, "[\0]"},
    
    {UNARY_PLUS, UNARY_PRE, 2, RIGHT, NORMAL, 0, "+"},
    {UNARY_MINUS, UNARY_PRE, 2, RIGHT, NORMAL, 0, "-"},
    {LOGICAL_NOT, UNARY_PRE, 2, RIGHT, NORMAL, 0, "!"},
    {BITWISE_NOT, UNARY_PRE, 2, RIGHT, NORMAL, 0, "~"},
    {DEREFERENCE, UNARY_PRE, 2, RIGHT, NORMAL, 0, "*"},
    {ADDRESS_OF, UNARY_PRE, 2, RIGHT, NORMAL, 0, "&"},
    
    {MULTIPLICATION, BINARY, 3, LEFT, NORMAL, 0, "*"},
    {DIVISION, BINARY, 3, LEFT, NORMAL, 0, "/"},
    {REMAINDER, BINARY, 3, LEFT, NORMAL, 0, "%"},

    {ADDITION, BINARY, 4, LEFT, NORMAL, 0, "+"},
    {SUBTRACTION, BINARY, 4, LEFT, NORMAL, 0, "-"},

    {BITWISE_LEFT_SHIFT, BINARY, 5, LEFT, NORMAL, 0, "<<"},
    {BITWISE_RIGHT_SHIFT, BINARY, 5, LEFT, NORMAL, 0, ">>"},

    {LESS_THAN, BINARY, 6, LEFT, NORMAL, 0, "<"},
    {LESS_EQUAL_TO, BINARY, 6, LEFT, NORMAL, 0, "<="},
    {GREATER_THAN, BINARY, 6, LEFT, NORMAL, 0, ">"},
    {GREATER_EQUAL_TO, BINARY, 6, LEFT, NORMAL, 0, ">="},

    {EQUAL, BINARY, 7, LEFT, NORMAL, 0, "=="},
    {NOT_EQUAL, BINARY, 7, LEFT, NORMAL, 0, "!="},

    {BITWISE_AND, BINARY, 8, LEFT, NORMAL, 0, "&"},
    {BITWISE_XOR, BINARY, 9, LEFT, NORMAL, 0, "^"},
    {BITWISE_OR, BINARY, 10, LEFT, NORMAL, 0, "|"},

    {LOGICAL_AND, BINARY, 11, LEFT, NORMAL, 0, "&&"},
    {LOGICAL_OR, BINARY, 12, LEFT, NORMAL, 0, "||"},

    {CAST, BINARY, 13, LEFT, NORMAL, 0, "to"},
    {BIT_CAST, BINARY, 13, LEFT, NORMAL, 0, "as"},

    {TERNARY, BINARY, 14, RIGHT, NORMAL, 0, "?"},
    {TERNARY_BODY, BINARY, 14, RIGHT, NORMAL, 0, ":"},

    {ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "="},
    {ADD_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "+="},
    {SUBTRACT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "-="},
    {PRODUCT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "*="},
    {QUOTIENT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "/="},
    {REMAINDER_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "%="},
    {BITWISE_LEFT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "<<="},
    {BITWISE_RIGHT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, ">>="},
    {BITWISE_AND_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "&="},
    {BITWISE_XOR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "^="},
    {BITWISE_OR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, 0, "|="},
};

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag);
char is_operator(const char * str);

const char * get_operator_runtime_name(enum Operators op);
void print_operator(const char * template, struct Operator * operator);
