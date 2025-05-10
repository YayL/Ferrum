#include "codegen/AST.h"
#include "common/common.h"
#include "common/hashmap.h"
#include "parser/parser.h"
#include "parser/types.h"
#include <stdio.h>
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

    if (parser->token->type == TOKEN_ID && !strcmp(parser->token->value, "impl")) {
        parser_eat(parser, TOKEN_ID);
        
        type->intrinsic = IImpl;
        type->name = parser->token->value;
        parser_eat(parser, TOKEN_ID);

        return ast;
    }
 
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
            struct Ast * temp_ast = parser_parse_type(parser);
            list_push(tuple_t->types, temp_ast);
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
        } else if (!strcmp(type->name, "bool")) {
            Numeric_T * num = init_intrinsic_type(INumeric);
            num->width = 1;
            num->type = NUMERIC_UNSIGNED;
            type->intrinsic = INumeric;
            type->ptr = num;
        } else if (is_template_type(ast, type->name)){
            type->intrinsic = ITemplate;
        } else {
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
                logger_log(format("{2i::} Numeric types must have a size bigger than 0", parser->token->line, parser->token->pos), PARSER, ERROR);
                parser->error = 1;
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
            Struct_T * _struct = calloc(1, sizeof(Struct_T));
            _struct->fields = init_list(sizeof(struct Ast *));
            return _struct;
        }
        case IEnum:
        {
            Enum_T * _enum = calloc(1, sizeof(Enum_T));
            return _enum;
        }
        case ITuple:
        {
            Tuple_T * tuple = malloc(sizeof(Tuple_T));
            tuple->types = init_list(sizeof(struct Ast *));
            return tuple;
        }
        case IImpl:
        {
            Impl_T * impl = calloc(1, sizeof(Impl_T));
            return impl;
        }
        case ITemplate:
        {
            Template_T * template = calloc(1, sizeof(Template_T));
            return template;
        }
    }
}

char __is_template_type(struct HashMap * map, char * name) {
    if (map == NULL)
        return 0;

    return hashmap_has(map, name);
}

char is_template_type(struct Ast * ast, char * name) {
    struct Ast * scope = get_scope(AST_FUNCTION, ast->scope); 
    
    return scope->type == AST_FUNCTION 
            && __is_template_type(((a_function *) scope->value)->template_types, name);
}

struct Ast * get_type(struct Ast * ast, char * name) {
    struct Ast * scope = get_scope(AST_MODULE, ast->scope), * temp;
    a_module * module = scope->value;
    a_struct * _struct;

    for (int i = 0; i < module->structures->size; ++i) {
        temp = list_at(module->structures, i);
        ASSERT1(temp->type == AST_SCOPE);
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

char check_types(Type * type1, Type * type2, struct HashMap * templates) {
    if (type1 == type2) {
        return 1;
    }

    if (type1->intrinsic == ITemplate) {
        // resolve template to corresponding type
        ASSERT(0, "Type 1 template is unfinished");
    }

    if (type2->intrinsic == ITemplate) {
        hashmap_set(templates, type_to_str(type2), type1);
        return 1;
    }

    if (type1->intrinsic == type2->intrinsic) {
        return is_equal_type(type1, type2, templates);
    }

    return is_implicitly_equal(type1, type2, templates);
}

char is_implicitly_equal(Type * type1, Type * type2, struct HashMap * self) {
    if (!is_equal_type(get_base_type(type1), get_base_type(type2), self))
        return 0;
 
    return 0;
}

char is_equal_type(Type * type1, Type * type2, struct HashMap * templates) {
    if (type1->intrinsic != type2->intrinsic) {
        return 0;
    }

    switch (type1->intrinsic) {
        case ITemplate:
            ASSERT1(0);
        case IRef:
        {
            Ref_T * ref1 = type1->ptr, * ref2 = type2->ptr;
 
            if (ref1->depth != ref2->depth) {
                return 0;
            }

            return check_types(ref1->basetype, ref2->basetype, templates);
        }
        case INumeric:
        {
            Numeric_T * num1 = type1->ptr, * num2 = type2->ptr;
            return num1->type == num2->type && num1->width == num2->width;
        }
        case ITuple:
        {
            Tuple_T * tuple1 = type1->ptr,
                    * tuple2 = type2->ptr;
            if (tuple1->types->size != tuple2->types->size) {
                return 0;
            }
            
            for (int i = 0; i < tuple1->types->size; ++i) {
                if (!check_types(((struct Ast *) list_at(tuple1->types, i))->value, 
                                 ((struct Ast *) list_at(tuple1->types, i))->value, 
                                 templates)) {
                    return 0;
                }
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
        case ITemplate:
        case IImpl:
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
        case ITemplate:
        case IImpl:
        case IEnum:
            return type_to_str(type);
        case IArray:
            return get_base_type_str(((Array_T *) type->ptr)->basetype);
        case IRef:
            return get_base_type_str(((Ref_T *) type->ptr)->basetype);
        case ITuple:
        {
            Tuple_T * tuple = type->ptr;
            if (tuple->types->size == 1) {
                return get_base_type_str(list_at(tuple->types, 0));
            }
            return NULL;
        }
    }
}

char * type_to_str(Type * type) {
    switch (type->intrinsic) {
        case ITemplate:
            return type->name;
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
        case IImpl:
        {
            return format("impl {s}", type->name);
        }
    }
}

Type * copy_type(Type * src) {
    Type * copy = malloc(sizeof(Type));
    memcpy(copy, src, sizeof(Type));

    copy->ptr = init_intrinsic_type(copy->intrinsic);
    switch (src->intrinsic) {
        case INumeric:
            memcpy(copy->ptr, src->ptr, sizeof(Numeric_T)); break;
        case IArray:
            memcpy(copy->ptr, src->ptr, sizeof(Array_T)); break;
        case IRef:
            memcpy(copy->ptr, src->ptr, sizeof(Ref_T)); break;
        default:
            ASSERT(0, format("Intrinsic type {i} is not implemented", copy->intrinsic));
    }
 
    return copy;
}

struct Ast * replace_self_in_type(struct Ast * ast, struct Ast * replacement_ast) {
    ASSERT(ast->type == AST_TYPE, "ast in replace_self_in_type is not AST_TYPE");
    ASSERT(replacement_ast->type == AST_TYPE, "replacement in replace_self_in_type is not AST_TYPE");

    struct Ast * ret = init_ast(AST_TYPE, ast->scope);
    free(ret->value);

    Type * to_replace = ast->value,
         * replacement = replacement_ast->value;

    Type * prev = NULL,
         * curr;

    do {
        switch (to_replace->intrinsic) {
            case ITemplate:
                curr = copy_type(replacement);
                to_replace = NULL;
                break;
            case IArray:
                curr = copy_type(to_replace);
                to_replace = (((Array_T *) to_replace->ptr)->basetype);
                break;
            case IRef:
                curr = copy_type(to_replace);
                to_replace = (((Ref_T *) to_replace->ptr)->basetype);
                break;
            default:
                ASSERT(0, format("Invalid intrinsic type: {i}", to_replace->intrinsic));
        }

        if (prev == NULL) {
            ret->value = curr;
        } else {
            switch (prev->intrinsic) {
                case IArray:
                    ((Array_T *) prev->ptr)->basetype = curr; break;
                case IRef:
                    ((Ref_T *) prev->ptr)->basetype = curr; break;
                default:
                    ASSERT(0, "Invalid intrinsic type");
            }
        }
        prev = curr;
    } while (to_replace != NULL); 

    return ret;
}
