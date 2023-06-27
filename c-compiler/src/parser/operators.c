#include "operators.h"

struct Operator str_to_operator(const char * str, enum OP_mode mode) {
    for (int i = 0; i < sizeof(op_conversion) / sizeof(op_conversion[0]); ++i) {
        if ((mode == OP_TYPE_ANY || op_conversion[i].mode == mode) && !strcmp(str, op_conversion[i].str))
            return op_conversion[i];
    }
    return op_conversion[0];
}
