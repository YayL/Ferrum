#pragma once

#include "common/memory/arena.h"
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

typedef struct Symbol_T { // struct/enum/impl/group
    struct type_info info;
    ID symbol_id;
    Arena templates;
} Symbol_T;

typedef struct Ref_T {
    struct type_info info;
    ID basetype_id;
    char depth;
    char is_mut;
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

typedef struct Place_T {
    struct type_info info;
    char is_mut;
    ID basetype_id;
} Place_T;

typedef struct Fn_T {
    struct type_info info;
    ID function_id;
    ID arg_type;
    ID ret_type;
} Fn_T;

typedef struct Variable_T {
    ID lower_bound;
    ID upper_bound;
} Variable_T;

const char * get_base_type_str(ID type);
ID get_base_type(ID type_id);
const char * type_to_str(ID type_id);
ID ast_get_type_of(ID node_id);
ID ast_to_type(ID node_id);

ID type_from_arena(Arena arena);

uint64_t type_id_to_hash(ID type);
char type_check_equal(ID type_id1, ID type_id2);

// struct AST * get_self_type(ID first_id, ID second_id);
// Type replace_self_in_type(ast, Type self);
