#pragma once

#include "common/common.h"

typedef struct Type {
    char * name;
    short size;
    void * ptr;
    enum intrinsic_type {
        INumeric, // isize, usize, fsize
        IBool, // true or false
        IPtr,
        IRef,
        IArray,
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

typedef struct Struct_T {
    struct List * fields;
} Struct_T;

Type * init_type();

Type * get_type(const char * type_str);
