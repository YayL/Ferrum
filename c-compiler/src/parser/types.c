#include "parser/parser.h"

struct Ast * parser_parse_type(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TYPE, parser->current_scope);
    a_type * type = ast->value,
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
        ref_t->depth = 1;

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

    // search for ID to see if it is a struct or enum. 
    // If so it could be a generic so gather those by checking for angular brackets
    
    struct Ast * marker = get_type(ast, type->name);
    struct List * list;

    if (marker != NULL) {
        switch (marker->type) {
            case AST_STRUCT:
            {
                // set basetype to be struct, and then set list to struct_t->generics or whatever it will be called
                Struct_T * struct_t = init_intrinsic_type(IStruct);
                list = struct_t->fields;
            } break;
            case AST_ENUM:
            {

            } break;
            default: {}
        }
        if (parser->token->type == TOKEN_LT) {
            parser_eat(parser, TOKEN_LT);

            while (parser->token->type != TOKEN_GT) {
                list_push(list, parser_parse_type(parser));
                if (parser->token->type == TOKEN_COMMA)
                    parser_eat(parser, TOKEN_COMMA);
            }

            parser_eat(parser, TOKEN_GT);
        }
    }

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
            Struct_T * Struct = calloc(1, sizeof(Struct_T));
            Struct->fields = init_list(sizeof(struct Ast *));
            return Struct;
        }
        case IEnum: {
            Enum_T * Enum = calloc(1, sizeof(Enum_T));
            return Enum;
        }
    }
}

struct Ast * get_type(struct Ast * ast, char * name) {
    struct Ast * scope = ast->scope, * temp;

    while (scope->type != AST_ROOT && scope->type != AST_MODULE)
        scope = scope->scope;
    
    ASSERT1(scope->type == AST_MODULE);

    a_module * module = scope->value;
    a_struct * _struct;

    for (int i = 0; i < module->structures->size; ++i) {
        temp = list_at(module->structures, i);
        _struct = temp->value;
        println("Name: {2s: = }", _struct->name, name);
        if (!strcmp(_struct->name, name)) {
            return temp;
        }
    }

    return NULL;
}

char * type_to_str(a_type * type) {
    switch (type->intrinsic) {
        case INumeric:
        case IStruct:
        case IEnum:
        {
            return type->name;
        }
        case IArray:
        {
            Array_T * array = type->ptr;
            return format("[]{s}", type_to_str(array->basetype));
        }
        case IRef:
        {
            Ref_T * ref = type->ptr;
            char * buf = malloc(sizeof(char) * (ref->depth + 1));
            
            int i = 0;
            while (i < ref->depth) {
                buf[i++] = '&';
            }
            buf[i] = 0;

            return format("{2s}", buf, type_to_str(ref->basetype));
        }
    }
}
