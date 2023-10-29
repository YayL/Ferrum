#include "parser/parser.h"
#include "parser/types.h"

struct Ast * parser_parse_type(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TYPE, parser->current_scope);
    Type * type = ast->value,
           * ref_type = NULL,
           * arr_type = NULL,
           * tuple_type = NULL;

    String * value = init_string("");
    unsigned int pos = 0;
    String temp;

    Ref_T * ref_t = NULL;
    Array_T * arr_t = NULL;
    Tuple_T * tuple_t = NULL;
        
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

    if (parser->token->type == TOKEN_LPAREN) {
        tuple_t = init_intrinsic_type(ITuple);
        parser_eat(parser, TOKEN_LPAREN);

        while (1) {
            list_push(tuple_t->types, parser_parse_type(parser));
            if (parser->token->type != TOKEN_COMMA)
                break;

            parser_eat(parser, TOKEN_COMMA);
        }
        parser_eat(parser, TOKEN_RPAREN);

        tuple_type = calloc(1, sizeof(Type));
        tuple_type->intrinsic = ITuple;
        tuple_type->ptr = tuple_t;

        ast->value = tuple_type;
    } else { 
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
        } else if (parser->token->type == TOKEN_LT) {
            logger_log(format("({2i::}) '{s}' is not a possible generic type. Must be a defined struct or enum.", parser->token->line, parser->token->pos, type->name), PARSER, ERROR);
            parser_eat(parser, TOKEN_LT);

            while (parser->token->type != TOKEN_GT) {
                parser_eat(parser, parser->token->type);
            }
            parser_eat(parser, TOKEN_GT);
        // check if not type Self as that is a special compile time type
        } else if (strcmp(type->name, "Self")) { // predfined types are parsed here
            Numeric_T * num = init_intrinsic_type(INumeric);
            switch (type->name[0]) {
                case 'i':
                    num->type = NUMERIC_SIGNED; break;
                case 'u':
                    num->type = NUMERIC_UNSIGNED; break;
                case 'f':
                    num->type = NUMERIC_FLOAT; break;
            }

            unsigned int size = atoi(type->name + 1);

            if (size == 0) {
                logger_log(format("{2u::} Unknown type: {s}", parser->token->line, parser->token->pos, type->name), PARSER, ERROR);
                exit(1);
            }

            num->width = size;
            type->ptr = num;
        }
    }


    if (arr_type != NULL) {
        arr_t->basetype = ast->value;
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
        case INumeric:
        {
            Numeric_T * numeric = calloc(1, sizeof(Numeric_T));
            return numeric;
        }
        case IRef:
        {
            Ref_T * ref = calloc(1, sizeof(Ref_T));
            return ref;
        }
        case IArray:
        {
            Array_T * arr = calloc(1, sizeof(Array_T));
            return arr;
        }
        case IStruct:
        {
            Struct_T * Struct = calloc(1, sizeof(Struct_T));
            Struct->fields = init_list(sizeof(struct Ast *));
            return Struct;
        }
        case IEnum:
        {
            Enum_T * Enum = calloc(1, sizeof(Enum_T));
            return Enum;
        }
        case ITuple:
        {
            Tuple_T * Tuple = malloc(sizeof(Tuple_T));
            Tuple->types = init_list(sizeof(struct Ast *));
            return Tuple;
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
        if (!strcmp(_struct->name, name)) {
            return temp;
        }
    }

    return NULL;
}

struct Ast * ast_to_type(struct Ast * ast) {
    struct Ast * node;
    switch (ast->type) {
        case AST_EXPR:
        {
            a_expr * expr = ast->value;

            struct Ast * node = init_ast(AST_TYPE, ast->scope);
            
            Type * type = node->value;
            Tuple_T * tuple = init_intrinsic_type(ITuple);
            
            type->intrinsic = ITuple;
            type->ptr = tuple;
            
            for (int i = 0; i < expr->children->size; ++i) {
                list_push(tuple->types, ((a_variable *) ((struct Ast *) list_at(expr->children, i))->value)->type);
            }

            return node;
        }
        default:
        {
            logger_log(format("AST type '{s}' is not an implemented type for type conversion", ast_type_to_str(ast->type)), PARSER, FATAL);
            exit(1);
        }
    }
}

char is_equal_type(Type * type1, Type * type2) {
    if (type1 == type2)
        return 1;

    if (type1->intrinsic != type2->intrinsic)
        return 0;

    switch (type1->intrinsic) {
        case ITuple:
        {
            Tuple_T * tuple1 = type1->ptr,
                    * tuple2 = type2->ptr;
            if (tuple1->types->size != tuple2->types->size)
                return 0;
            
            for (int i = 0; i < tuple1->types->size; ++i) {
                if (!is_equal_type(((struct Ast *) list_at(tuple1->types, i))->value, ((struct Ast *) list_at(tuple1->types, i))->value))
                    return 0;
            }
        } break;
        default:
        {
            logger_log(format("{i} is not an implemented intrinsic for checking type equivalance", type1->intrinsic), CHECKER, FATAL);
            exit(1);
        }
    }

    return 1;
}

char * type_to_str(Type * type) {
    switch (type->intrinsic) {
        case IStruct:
        case IEnum:
        {
            return type->name;
        }
        case INumeric:
        {
            Numeric_T * num = type->ptr;
            char c = 'i';
            switch (num->type) {
                case NUMERIC_SIGNED:
                    break;
                case NUMERIC_UNSIGNED:
                    c = 'u'; break;
                case NUMERIC_FLOAT:
                    c = 'f'; break;
                default:
                    logger_log(format("Invalid numeric type: {i}", num->type), CHECKER, ERROR);
            }
            return format("{c}{u}", c, num->width);
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
        case ITuple:
        {
            Tuple_T * tuple = type->ptr;
            Type * temp = ((struct Ast *) list_at(tuple->types, 0))->value;
            char * buf = format("({s}", type_to_str(temp));

            for (int i = 1; i < tuple->types->size; ++i) {
                temp = ((struct Ast *) list_at(tuple->types, i))->value;
                buf = format("{2s:, }", buf, type_to_str(temp));
            }

            return format("{s})", buf);
        }
    }
}
