#pragma once

#include "common/memory/arena.h"
#include "common/ID.h"

#define VOID_TYPE ((ID) { .type = ID_VOID_TYPE, .id = 0 })

struct type_info {
    ID type_id;
    char is_mut;
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

typedef struct Symbol_T { // any struct/enum types
    struct type_info info;
    ID symbol_id;
    Arena templates;
} Symbol_T;

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

typedef struct Group_T {
    struct type_info info;
    ID symbol_id;
} Group_T;

void type_init_intrinsic_type(enum id_type type, void * type_ref);

const char * get_base_type_str(ID type);
ID get_base_type(ID type_id);
char * type_to_str(ID type_id);
ID ast_get_type_of(ID node_id);
ID ast_to_type(ID node_id);

ID type_from_arena(Arena arena);

// struct AST * get_self_type(ID first_id, ID second_id);
// Type replace_self_in_type(ast, Type self);
