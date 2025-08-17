#include "parser/operators.h"
#include "common/string.h"
#include "tables/interner.h"

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

#define OPERATOR_GET_RUNTIME_NAME(KEY) "#OP_" #KEY
#define OPERATOR_GET(KEY) op_conversion[KEY]
#define OPERATOR_INTERN(KEY, ...) \
    OPERATOR_GET(KEY).intern_id = interner_intern(STRING_FROM_LITERAL(OPERATOR_GET_RUNTIME_NAME(KEY)));

void operators_intern() {
    OPERATORS_LIST(OPERATOR_INTERN)
}

unsigned int operator_get_intern_id(enum Operators op) {
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
