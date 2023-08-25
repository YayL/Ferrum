#include "parser/operators.h"

struct Operator str_to_operator(const char * str, enum OP_mode mode, char * enclosed_flag) {
    const char * op_str;
    for (int i = 0; i < sizeof(op_conversion) / sizeof(op_conversion[0]); ++i) {
        op_str = op_conversion[i].str;

        if (op_conversion[i].enclosed == ENCLOSED && op_conversion[i].mode == mode) {
            if (!strcmp(str, op_str)) {
                *enclosed_flag = 0;
                return op_conversion[i];
            }
            else if (!strcmp(str, op_str + strlen(op_str) + 1)) {
                *enclosed_flag = 1;
                return op_conversion[i];
            }
        }

        if ((mode == OP_TYPE_ANY || op_conversion[i].mode == mode) && !strcmp(str, op_str))
            return op_conversion[i];
    }
    return op_conversion[0];
}

void print_operator(const char * template, struct Operator * operator) {
    char * op_str = format("<op='{s}', precedence='{u}', mode='{s}', enclosed='{b}', associativity='{s}'>",
                            operator->str,
                            operator->precedence,
                            operator->mode == BINARY ? "Binary" : "Unary",
                            operator->enclosed == ENCLOSED,
                            operator->associativity == LEFT ? "Left" : "Right"
                            );
    
    print(template, op_str);
    free(op_str);
}
