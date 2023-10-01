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
    LESS_THAN_BETWEEN, // a < var < b
    GREATER_THAN_BETWEEN, // a > var > b
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
    char * str;
} op_conversion [] = {
    {OP_NOT_FOUND, UNARY_PRE, 0, LEFT, NORMAL, ""},
    {PARENTHESES, UNARY_PRE, 0, LEFT, ENCLOSED, "(\0)"},
    //{BRACKETS, UNARY_PRE, 0, LEFT, ENCLOSED, "[\0]"},
    
    {INCREMENT, UNARY_PRE, 1, LEFT, NORMAL, "++"},
    {INCREMENT, UNARY_POST, 1, LEFT, NORMAL, "++"},
    {DECREMENT, UNARY_PRE, 1, LEFT, NORMAL, "--"},
    {DECREMENT, UNARY_POST, 1, LEFT, NORMAL, "--"},

    {MEMBER_ACCESS, BINARY, 1, LEFT, NORMAL, "."},
    {ARROW, BINARY, 1, LEFT, NORMAL, "->"},
    {CALL, BINARY, 1, LEFT, ENCLOSED, "(\0)"},
    {SUBSCRIPT, BINARY, 1, LEFT, ENCLOSED, "[\0]"},
    
    {UNARY_PLUS, UNARY_PRE, 2, RIGHT, NORMAL, "+"},
    {UNARY_MINUS, UNARY_PRE, 2, RIGHT, NORMAL, "-"},
    {LOGICAL_NOT, UNARY_PRE, 2, RIGHT, NORMAL, "!"},
    {BITWISE_NOT, UNARY_PRE, 2, RIGHT, NORMAL, "~"},
    {DEREFERENCE, UNARY_PRE, 2, RIGHT, NORMAL, "*"},
    {ADDRESS_OF, UNARY_PRE, 2, RIGHT, NORMAL, "&"},
    
    {MULTIPLICATION, BINARY, 3, LEFT, NORMAL, "*"},
    {DIVISION, BINARY, 3, LEFT, NORMAL, "/"},
    {REMAINDER, BINARY, 3, LEFT, NORMAL, "%"},

    {ADDITION, BINARY, 4, LEFT, NORMAL, "+"},
    {SUBTRACTION, BINARY, 4, LEFT, NORMAL, "-"},

    {BITWISE_LEFT_SHIFT, BINARY, 5, LEFT, NORMAL, "<<"},
    {BITWISE_RIGHT_SHIFT, BINARY, 5, LEFT, NORMAL, ">>"},

    {LESS_THAN, BINARY, 6, LEFT, NORMAL, "<"},
    {LESS_EQUAL_TO, BINARY, 6, LEFT, NORMAL, "<="},
    {GREATER_THAN, BINARY, 6, LEFT, NORMAL, ">"},
    {GREATER_EQUAL_TO, BINARY, 6, LEFT, NORMAL, ">="},

    {EQUAL, BINARY, 7, LEFT, NORMAL, "=="},
    {NOT_EQUAL, BINARY, 7, LEFT, NORMAL, "!="},

    {BITWISE_AND, BINARY, 8, LEFT, NORMAL, "&"},
    {BITWISE_XOR, BINARY, 9, LEFT, NORMAL, "^"},
    {BITWISE_OR, BINARY, 10, LEFT, NORMAL, "|"},

    {LOGICAL_AND, BINARY, 11, LEFT, NORMAL, "&&"},
    {LOGICAL_OR, BINARY, 12, LEFT, NORMAL, "||"},

    {CAST, BINARY, 13, LEFT, NORMAL, "to"},
    {BIT_CAST, BINARY, 13, LEFT, NORMAL, "as"},

    {TERNARY, BINARY, 14, RIGHT, NORMAL, "?"},
    {TERNARY_BODY, BINARY, 14, RIGHT, NORMAL, ":"},

    {ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "="},
    {ADD_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "+="},
    {SUBTRACT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "-="},
    {PRODUCT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "*="},
    {QUOTIENT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "/="},
    {REMAINDER_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "%="},
    {BITWISE_LEFT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "<<="},
    {BITWISE_RIGHT_SHIFT_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, ">>="},
    {BITWISE_AND_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "&="},
    {BITWISE_XOR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "^="},
    {BITWISE_OR_ASSIGNMENT, BINARY, 15, RIGHT, NORMAL, "|="},
};

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag);
void print_operator(const char * template, struct Operator * operator);
