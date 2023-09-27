#include "parser/parser.h"

struct Ast * parser_parse_type(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TYPE, parser->current_scope);
    a_type * type = calloc(1, sizeof(Type)),
           * ref_type = NULL,
           * arr_type = NULL;

    String * value = init_string("");
    unsigned int pos = 0;
    String temp;

    Ref_T * ref_t = NULL;
    Array_T * arr_t = NULL;
        
    if (parser->token->type == TOKEN_AMPERSAND) {
        parser_eat(parser, TOKEN_AMPERSAND);
        ref_t = init_intrinsic_type(IRef);

        while (parser->token->type == TOKEN_AMPERSAND) {
            parser_eat(parser, TOKEN_AMPERSAND);
            ref_t->depth++;
        }

        ref_type = calloc(1, sizeof(Type));
        ref_type->intrinsic = IRef;
        ref_type->ptr = ref_t;
    }

    if (parser->token->type == TOKEN_LBRACKET) {
        arr_t = init_intrinsic_type(IArray);
        parser_eat(parser, TOKEN_LBRACKET);

        arr_t->basetype = type;
        arr_t->size = -1;
        
        if (parser->token->type == TOKEN_INT) {
            arr_t->size = atoi(parser->token->value);
            parser_eat(parser, TOKEN_INT);
        } else if (parser->token->type == TOKEN_UNDERSCORE) {
            logger_log("Slices have not been implemented yet", PARSER, ERROR);
            exit(1);
        } else {
            arr_t->size = -1;
        }

        parser_eat(parser, TOKEN_RBRACKET);

        arr_type = calloc(1, sizeof(Type));
        arr_type->intrinsic = IArray;
        arr_type->ptr = arr_t;
    }
    
    type->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);
    
    ast->value = type;

    if (arr_type != NULL) {
        ast->value = arr_type;
    }

    if (ref_type != NULL) {
        ref_t->basetype = ast->value;
        ast->value = ref_type;
    }

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

