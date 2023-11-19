#include "codegen/AST.h"
#include "parser/parser.h"
#include "parser/types.h"
#include <string.h>

struct Ast * parser_parse_type(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TYPE, parser->current_scope);
    Type * type = ast->value,
           * ref_type = NULL,
           * arr_type = NULL,
           * tuple_type = NULL;
    
    type->intrinsic = 0;

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
                        type->intrinsic = IStruct;
                    } break;
                case AST_ENUM:
                    {
                        type->intrinsic = IEnum;
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
        } else if (!strcmp(type->name, "Self")) { // predfined types are parsed here
            type->intrinsic = ISelf;
        } else if (!strcmp(type->name, "bool")) {
            Numeric_T * num = init_intrinsic_type(INumeric);
            num->width = 1;
            num->type = NUMERIC_UNSIGNED;
            type->intrinsic = INumeric;
            type->ptr = num;
        }else {
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
            type->intrinsic = INumeric;
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
        case ISelf:
            logger_log("Intrinsic ISelf can not be initiated.", PARSER, ERROR);
            exit(1);
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
    switch (ast->type) {
        case AST_EXPR:
        {
            a_expr * expr = ast->value;

            if (expr->children->size == 1) {
                return ast_get_type_of(list_at(expr->children, 0));
            }

            struct Ast * node = init_ast(AST_TYPE, ast->scope),
                       * temp;
            
            Type * type = node->value;
            Tuple_T * tuple = init_intrinsic_type(ITuple);
            
            type->intrinsic = ITuple;
            type->ptr = tuple;
            
            for (int i = 0; i < expr->children->size; ++i) {
                list_push(tuple->types, ast_get_type_of(list_at(expr->children, i)));
            }

            return node;
        }
        case AST_STRUCT:
        {
            a_struct * _struct = ast->value;
            struct Ast * node = init_ast(AST_TYPE, ast->scope);

            Type * type = node->value;
            
            type->name = _struct->name;
            type->intrinsic = IStruct;
            type->ptr = NULL;

            return node;
        }
        default:
        {
            logger_log(format("'{u}' is not an implemented type for type conversion", ast_type_to_str(ast->type)), PARSER, FATAL);
            exit(1);
        }
    }
}

char is_implicitly_equal(Type * type1, Type * type2, Type * self) {
    if (!is_equal_type(get_base_type(type1), get_base_type(type2), self))
        return 0;

    char r1 = type1->intrinsic == IRef, r2 = type2->intrinsic == IRef;

    if (r1 && r2) {
        int diff = ((Ref_T *) type1->ptr)->depth - ((Ref_T *) type2->ptr)->depth;
        if (diff == 1) {
            type1->implicit = IE_DEREFERENCE;
            return 1;
        } else if (diff == -1) {
            type1->implicit = IE_REFERENCE;
            return 1;
        }
    } else if (r1 && !r2) {
        if (((Ref_T *) type1->ptr)->depth == 1) {
            type1->implicit = IE_DEREFERENCE; 
            return 1;
        } 
    } else if (!r1 && r2) {
        if (((Ref_T *) type2->ptr)->depth == 1) {
            type1->implicit = IE_REFERENCE;
            return 1;
        }
    }
    
    return 0;
}

char is_equal_type(Type * type1, Type * type2, Type * self) {
    if (type1 == type2)
        return 1;

    if (type1->intrinsic != type2->intrinsic) {
        if (self != NULL && ((type1->intrinsic == ISelf && is_equal_type(self, type2, NULL)) || (type2->intrinsic == ISelf && is_equal_type(type1, self, NULL))))
            return 1;
        return 0;
    }

    switch (type1->intrinsic) {
        case ISelf:
            break;
        case INumeric:
        {
            Numeric_T * num1 = type1->ptr, * num2 = type2->ptr;
            return num1->type == num2->type && num1->width == num2->width;
        }
        case ITuple:
        {
            Tuple_T * tuple1 = type1->ptr,
                    * tuple2 = type2->ptr;
            if (tuple1->types->size != tuple2->types->size)
                return 0;
            
            for (int i = 0; i < tuple1->types->size; ++i) {
                if (!is_equal_type(((struct Ast *) list_at(tuple1->types, i))->value, ((struct Ast *) list_at(tuple1->types, i))->value, NULL))
                    return 0;
            }
        } break;
        default:
        {
            logger_log(format("{u} is not an implemented intrinsic for checking type equivalance", type1->intrinsic), CHECKER, FATAL);
            exit(1);
        }
    }

    return 1;
}

struct List * ast_to_ast_type_list(struct Ast * ast) {
    ASSERT(ast->type == AST_TYPE, "type_to_type_list not type AST");
    Type * type = ast->value;
    switch (type->intrinsic) {
        case ITuple:
            return ((Tuple_T *) type->ptr)->types;
        default:
        {
            struct List * list = init_list(sizeof(struct Ast *));
            list_push(list, ast);
            return list;
        }
    }
}

Type * get_base_type(Type * type) {
    switch (type->intrinsic) {
        case IStruct:
        case INumeric:
        case ISelf:
        case IEnum:
        case ITuple:
            return type;
        case IArray:
            return get_base_type(((Array_T *) type->ptr)->basetype);
        case IRef:
            return get_base_type(((Ref_T *) type->ptr)->basetype);
    }
}

const char * get_base_type_str(Type * type) {
    switch (type->intrinsic) {
        case IStruct:
        case INumeric:
        case ISelf:
        case IEnum:
            return type_to_str(type);
        case IArray:
            return get_base_type_str(((Array_T *) type->ptr)->basetype);
        case IRef:
            return get_base_type_str(((Ref_T *) type->ptr)->basetype);
        case ITuple:
        {
            Tuple_T * tuple = type->ptr;
            if (tuple->types->size == 1)
                return list_at(tuple->types, 0);
            return NULL;
        }
    }
}

char * type_to_str(Type * type) {
    switch (type->intrinsic) {
        case ISelf:
            return "Self";
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
            struct Ast * temp = list_at(tuple->types, 0);
            char * buf = format("({s}", get_type_str(temp));

            for (int i = 1; i < tuple->types->size; ++i) {
                temp = list_at(tuple->types, i);
                buf = format("{2s:, }", buf, get_type_str(temp));
            }

            return format("{s})", buf);
        }
    }
}

Type * __replace_self_in_type_using_type(Type * type, struct Ast * self) {
    switch (type->intrinsic) {
        case INumeric:
        case IStruct:
        case IEnum:
            return type;
        case ISelf:
            return self->value;
        case IRef:
        {
            Type * ref = init_ast_of_type(AST_TYPE);
            memcpy(ref, type, sizeof(Type));
            
            Ref_T * ref_t = init_intrinsic_type(IRef);
            memcpy(ref_t, type->ptr, sizeof(Ref_T));
            
            ref_t->basetype = __replace_self_in_type_using_type(ref_t->basetype, self);
            ref->ptr = ref_t;

            return ref;
        }
        case IArray:
        {
            Type * arr = init_ast_of_type(AST_TYPE);
            memcpy(arr, type, sizeof(Type));
            
            Array_T * arr_t = init_intrinsic_type(IArray);
            memcpy(arr_t, type->ptr, sizeof(Array_T));
            
            arr_t->basetype = __replace_self_in_type_using_type(arr_t->basetype, self);
            arr->ptr = arr_t;

            return arr;
        }
        case ITuple:
        {
            Type * tuple = init_ast_of_type(AST_TYPE);
            memcpy(tuple, type, sizeof(Type));
            
            Tuple_T * tuple_t = init_intrinsic_type(ITuple),
                    * r_tuple_t = type->ptr;
            
            
            for (int i = 0; i < r_tuple_t->types->size; ++i) {
                list_push(tuple_t->types, replace_self_in_type(list_at(r_tuple_t->types, i), self));
            }

            tuple->ptr = tuple_t;

            return tuple;
        }
    }
}

struct Ast * replace_self_in_type(struct Ast * ast, struct Ast * self) {
    if (ast == NULL)
        return NULL;
    switch (((a_type *) ast->value)->intrinsic) {
        case INumeric:
        case IStruct:
        case IEnum:
            return ast;
        case ISelf:
        {
            return self;
        }
        case IRef:
        case IArray:
        case ITuple:
        {
            struct Ast * node = init_ast(AST_TYPE, ast->scope);
            node->value = __replace_self_in_type_using_type(ast->value, self);
            return node;
        }
    }

}

Type * __get_self_type(Type * type1, Type * type2) {
    if (type2->intrinsic == ISelf)
        return type1;

    switch (type1->intrinsic) {
        case IRef:
        {
            Ref_T * ref1 = type1->ptr, * ref2 = type2->ptr;

            if (ref1->depth != ref2->depth)
                break;

            return __get_self_type(ref1->basetype, ref2->basetype);
        }
    }

    return NULL;
}

struct Ast * get_self_type(struct Ast * first, struct Ast * second) {
    ASSERT1(first->type == AST_TYPE && second->type == AST_TYPE);
    Type * type1 = first->value, * type2 = second->value;

    if (type2->intrinsic == ISelf)
        return first;

    ASSERT(type1->intrinsic == type2->intrinsic, "get_self_type not same intrinsic");

    Type * res = __get_self_type(type1, type2);
    if (res != NULL) {
        struct Ast * ast = init_ast(AST_TYPE, first->scope);
        ast->value = res;
        return ast;
    }

    logger_log("get_self_type unable to find self representative", CHECKER, ERROR);
    exit(1);
}
