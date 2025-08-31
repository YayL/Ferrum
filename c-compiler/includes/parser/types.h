#pragma once

#include "common/arena.h"
#include "common/ID.h"

#define VOID_TYPE ((ID) { .type = ID_VOID_TYPE, .id = 0 })

struct type_info {
    ID type_id;
};

typedef struct Numeric_T {
    struct type_info info;
    unsigned short width;
    enum Numeric_T_TYPE {
        NUMERIC_UNSIGNED,
        NUMERIC_SIGNED,
        NUMERIC_FLOAT,
    } type;
} Numeric_T;

typedef struct Type_T {
    struct type_info info;
    ID symbol_id;
} Type_T;

typedef struct Ref_T {
    struct type_info info;
    ID basetype_id;
    char depth;
} Ref_T;

typedef struct Array_T {
    struct type_info info;
    ID basetype_id;
    unsigned int size;
} Array_T;

typedef struct Tuple_T {
    struct type_info info;
    Arena types;
} Tuple_T;

typedef struct Impl_T {
    struct type_info info;
    ID symbol_id;
    ID implementees_id;
} Impl_T;

void type_init_intrinsic_type(enum id_type type, void * type_ref);

const char * get_base_type_str(ID type);
ID get_base_type(ID type_id);
char * type_to_str(ID type_id);
ID ast_get_type_of(ID node_id);
ID ast_to_type(ID node_id);

char is_template_type(ID current_scope, ID name_id);
ID get_type(ID node_id, ID name_id);
const Arena type_to_type_arena(ID type_id);

char is_implicitly_equal(ID type1, ID type2);
char is_equal_type(ID type1, ID type2);

// struct AST * get_self_type(ID first_id, ID second_id);
// Type replace_self_in_type(ast, Type self);
