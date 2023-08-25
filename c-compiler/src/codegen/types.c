#include "codegen/types.h"

Type * init_type() {
    Type * type = calloc(1, sizeof(Type));
    return type;
}

Type * get_type(const char * type_str) {
    Type * type = init_type();
}
