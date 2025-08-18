#pragma once

#include "common/common.h"
#include "common/hashmap.h"
#include <sys/types.h>

#ifndef TYPES_FOREACH
#define TYPES_FOREACH(f) \
    f(INumeric, Numeric_T, numeric) \
    f(IType, Type_T, type) \
    f(ITuple, Tuple_T, tuple) \
    f(IImpl, Impl_T, implementation) \
    f(IArray, Array_T, array) \
    f(IRef, Ref_T, ref)
#endif

#define TYPE_ENUM_EL(ENUM_NAME, ...) ENUM_NAME,
#define TYPE_UNION_EL(_, STRUCT_NAME, NAME) STRUCT_NAME NAME;
#define TYPE_FORWARD_DECLARE(_, STRUCT_NAME, ...) typedef struct STRUCT_NAME STRUCT_NAME;

#define UNKNOWN_TYPE (Type) {.intrinsic = IUnknown}

typedef struct Type Type;

typedef struct Numeric_T {
    unsigned short width;
    enum Numeric_T_TYPE {
        NUMERIC_UNSIGNED,
        NUMERIC_SIGNED,
        NUMERIC_FLOAT,
    } type;
} Numeric_T;

typedef struct Type_T {} Type_T;

typedef struct Ref_T {
    Type * basetype;
    char depth;
} Ref_T;

typedef struct Array_T {
    Type * basetype;
    unsigned int size;
} Array_T;

typedef struct Tuple_T {
    Arena types;
} Tuple_T;

typedef struct Impl_T {
    struct AST * type;
} Impl_T;

typedef struct Type {
    unsigned int name_id;
    enum intrinsic_type {
        IUnknown,
        TYPES_FOREACH(TYPE_ENUM_EL)
    } intrinsic;
    union intrinsic_union {
        TYPES_FOREACH(TYPE_UNION_EL)
    } value;
    short size;
} Type;

union intrinsic_union init_intrinsic_type(enum intrinsic_type type);
const char * get_base_type_str(Type type);
Type get_base_type(Type type);
char * type_to_str(Type type);
Type * ast_get_type_of(struct AST * ast);
Type ast_to_type(struct AST * ast);

char is_template_type(struct AST * current_scope, unsigned int name_id);
struct AST * get_type(struct AST * ast, unsigned int name_id);
Arena type_to_type_arena(Type ast);

char check_types(Type type1, Type type2, struct hashmap * templates);
char is_implicitly_equal(Type type1, Type type2);
char is_equal_type(Type type1, Type type2);

Type copy_type(Type src);

struct AST * get_self_type(struct AST * first, struct AST * second);
Type replace_self_in_type(Type ast, Type self);
