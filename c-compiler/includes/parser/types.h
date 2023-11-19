#pragma once

#include "common/common.h"

typedef struct Type {
    char * name;
    short size;
    void * ptr;
    enum implicitly_equal {
        IE_EQUAL,
        IE_REFERENCE,
        IE_DEREFERENCE
    } implicit;
    enum intrinsic_type {
        INumeric, // isize, usize, fsize
        IArray,
        IRef,
        IStruct,
        IEnum,
        ITuple,
        ISelf
    } intrinsic;
} Type;

struct Field {
    char * name;
    Type * type;
};

typedef struct Numeric_T {
    unsigned short width;
    enum Numeric_T_TYPE {
        NUMERIC_UNSIGNED,
        NUMERIC_SIGNED,
        NUMERIC_FLOAT,
    } type;
} Numeric_T;

typedef struct Ref_T {
    Type * basetype;
    char depth;
} Ref_T;

typedef struct Array_T {
    Type * basetype;
    unsigned int size;
} Array_T;

typedef struct Struct_T {
    struct List * fields;
} Struct_T;

typedef struct Enum_T {
    struct List * fields;
} Enum_T;

typedef struct Tuple_T {
    struct List * types;
} Tuple_T;

void * init_intrinsic_type(enum intrinsic_type type);
const char * get_base_type_str(Type * type);
Type * get_base_type(Type * type);
char * type_to_str(Type * type);

struct Ast * get_type(struct Ast * ast, char * name);
char is_implicitly_equal(Type * type1, Type * type2, Type * self);
char is_equal_type(Type * type1, Type * type2, Type * self);
struct Ast * ast_to_type(struct Ast * ast);

struct Ast * get_self_type(struct Ast * first, struct Ast * second);
struct Ast * replace_self_in_type(struct Ast * ast, struct Ast * self);
