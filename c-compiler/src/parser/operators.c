#include "parser/operators.h"

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag) {
    const char * op_str;
    for (int i = 0; i < sizeof(op_conversion) / sizeof(op_conversion[0]); ++i) {
        op_str = op_conversion[i].str;

        if (op_conversion[i].enclosed == ENCLOSED && op_conversion[i].mode == mode) {
            if (!strcmp(str, op_str)) {
                if (enclosed_flag != NULL)
                    *enclosed_flag = 0;
                return op_conversion[i];
            } else if (!strcmp(str, op_str + strlen(op_str) + 1)) {
                if (enclosed_flag != NULL)
                    *enclosed_flag = 1;
                return op_conversion[i];
            }
        }

        if ((mode == OP_TYPE_ANY || op_conversion[i].mode == mode) && !strcmp(str, op_str))
            return op_conversion[i];
    }
    return op_conversion[0];
}

char is_operator(const char * str) {
    for (int i = 0; i < sizeof(op_conversion) / sizeof(op_conversion[0]); ++i) {
        if (!strcmp(str, op_conversion[i].str)) {
            return op_conversion[i].key != OP_NOT_FOUND;
        } else if (op_conversion[i].enclosed && !strcmp(str, op_conversion[i].str + strlen(op_conversion[i].str) + 1)) {
            return 1;
        }
    }

    return 0;
}

const char * get_operator_runtime_name(enum Operators op) {
    switch (op) {
        case OP_NOT_FOUND: return "#OP_OP_NOT_FOUND";
        case PARENTHESES: return "#OP_PARENTHESES";
        case ARRAY: return "#OP_ARRAY";
        case PRE_INCREMENT: return "#OP_PRE_INCREMENT";
        case PRE_DECREMENT: return "#OP_PRE_DECREMENT";
        case POST_INCREMENT: return "#OP_POST_INCREMENT";
        case POST_DECREMENT: return "#OP_POST_DECREMENT";
        case MEMBER_ACCESS: return "#OP_MEMBER_ACCESS";
        case MEMBER_ACCESS_PTR: return "#OP_MEMBER_ACCESS_PTR";
        case CALL: return "#OP_CALL";
        case SUBSCRIPT: return "#OP_SUBSCRIPT";

        case UNARY_PLUS: return "#OP_UNARY_PLUS";
        case UNARY_MINUS: return "#OP_UNARY_MINUS";
        case LOGICAL_NOT: return "#OP_LOGICAL_NOT";
        case BITWISE_NOT: return "#OP_BITWISE_NOT";
        case DEREFERENCE: return "#OP_DEREFERENCE";
        case ADDRESS_OF: return "#OP_ADDRESS_OF";
        case CAST: return "#OP_CAST";
        case BIT_CAST: return "#OP_BIT_CAST";

        case MULTIPLICATION: return "#OP_MULTIPLICATION";
        case DIVISION: return "#OP_DIVISION";
        case REMAINDER: return "#OP_REMAINDER";

        case ADDITION: return "#OP_ADDITION";
        case SUBTRACTION: return "#OP_SUBTRACTION";

        case BITWISE_LEFT_SHIFT: return "#OP_BITWISE_LEFT_SHIFT";
        case BITWISE_RIGHT_SHIFT: return "#OP_BITWISE_RIGHT_SHIFT";

        case LESS_THAN: return "#OP_LESS_THAN";
        case LESS_EQUAL_TO: return "#OP_LESS_EQUAL_TO";
        case GREATER_THAN: return "#OP_GREATER_THAN";
        case GREATER_EQUAL_TO: return "#OP_GREATER_EQUAL_TO";

        case EQUAL: return "#OP_EQUAL";
        case NOT_EQUAL: return "#OP_NOT_EQUAL";

        case BITWISE_AND: return "#OP_BITWISE_AND";
        case BITWISE_XOR: return "#OP_BITWISE_XOR";
        case BITWISE_OR: return "#OP_BITWISE_OR";

        case LOGICAL_AND: return "#OP_LOGICAL_AND";
        case LOGICAL_OR: return "#OP_LOGICAL_OR";

        case TERNARY: return "#OP_TERNARY";
        case TERNARY_BODY: return "#OP_TERNARY_BODY";

        case ASSIGNMENT: return "#OP_ASSIGNMENT";
        case ADD_ASSIGNMENT: return "#OP_ADD_ASSIGNMENT";
        case SUBTRACT_ASSIGNMENT: return "#OP_SUBTRACT_ASSIGNMENT";
        case PRODUCT_ASSIGNMENT: return "#OP_PRODUCT_ASSIGNMENT";
        case QUOTIENT_ASSIGNMENT: return "#OP_QUOTIENT_ASSIGNMENT";
        case REMAINDER_ASSIGNMENT: return "#OP_REMAINDER_ASSIGNMENT";
        case BITWISE_LEFT_SHIFT_ASSIGNMENT: return "#OP_BITWISE_LEFT_SHIFT_ASSIGNMENT";
        case BITWISE_RIGHT_SHIFT_ASSIGNMENT: return "#OP_BITWISE_RIGHT_SHIFT_ASSIGNMENT";
        case BITWISE_AND_ASSIGNMENT: return "#OP_BITWISE_AND_ASSIGNMENT";
        case BITWISE_XOR_ASSIGNMENT: return "#OP_BITWISE_XOR_ASSIGNMENT";
        case BITWISE_OR_ASSIGNMENT: return "#OP_BITWISE_OR_ASSIGNMENT";
    }
}

void print_operator(const char * template, struct Operator * operator) {
    char * op_str = format("<op=({i})'{s}', precedence='{u}', mode='{s}', enclosed='{b}', associativity='{s}'>",
                            operator->key,
                            operator->str,
                            operator->precedence,
                            operator->mode == BINARY ? "Binary" : "Unary",
                            operator->enclosed == ENCLOSED,
                            operator->associativity == LEFT ? "Left" : "Right"
                            );
    
    print(template, op_str);
    free(op_str);
}
