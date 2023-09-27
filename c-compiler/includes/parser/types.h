#pragma once

#include "common/common.h"

typedef struct Type {
    char * name;
    short size;
    void * ptr;
    enum intrinsic_type {
        INumeric, // isize, usize, fsize
        IArray,
        IRef,
        IStruct,
        IEnum
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

typedef struct Product_T {
    struct List * fields;
} Product_T;

typedef struct Enum_T {
    struct List * fields;
} Enum_T;

void * init_intrinsic_type(enum intrinsic_type type);
