#include "parser/parser.h"

struct Ast * parser_parse_type(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TYPE, parser->current_scope);
    a_type * type = ast->value;

    String * value = init_string("");
    unsigned int pos = 0;
    String temp;
    
    /* do { */
    /*     temp._ptr = parser->token->value; */
    /*     temp.size = parser->token->length; */
    /*     string_concat(value, &temp); */
    /*      */
    /*     pos = parser->token->pos + parser->token->length - 1; */
    /*     parser_eat(parser, parser->token->type); */
    /* } while (pos == parser->token->pos); */
    
    println("{s}", parser->token->value);

    if (parser->token->type == TOKEN_OP && parser->token->value[0] == '&') {
        type->ptr = init_intrinsic_type(IRef);
    }

    println("Type: {s}", value->_ptr);

    return ast;
}

void * init_intrinsic_type(enum intrinsic_type type) {
    switch (type) {
        case INumeric: {
            Numeric_T * numeric = calloc(1, sizeof(Numeric_T));
            return numeric;
        }
        case IRef: {
            Ref_T * ref = calloc(1, sizeof(Ref_T));
            return ref;
        }
        case IArray: {
            Array_T * arr = calloc(1, sizeof(Array_T));
            return arr;
        }
        case IStruct: {
            Product_T * Struct = calloc(1, sizeof(Product_T));
            return Struct;
        }
        case IEnum: {
            Enum_T * Enum = calloc(1, sizeof(Enum_T));
            return Enum;
        }
    }
}

