#include "parser/types.h"

#include "common/common.h"
#include "codegen/AST.h"
#include "parser/keywords.h"
#include "parser/parser.h"
#include "parser/token.h"
#include "tables/interner.h"

Type parser_parse_type(struct Parser * parser) {
    Type type = UNKNOWN_TYPE;

    switch (parser->token->type) {
    case TOKEN_ID:
        if (parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_IMPL)) {
            parser_eat(parser, TOKEN_ID);

            type.intrinsic = IImpl;
            type.name_id = parser->token->value.interner_id;
            parser_eat(parser, TOKEN_ID);
        } else if (parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_STRUCT)) {
            parser_eat(parser, TOKEN_ID);

            type.intrinsic = IStruct;
            type.name_id = parser->token->value.interner_id;
            parser_eat(parser, TOKEN_ID);
        } else if (parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_ENUM)) {
            parser_eat(parser, TOKEN_ID);

            type.intrinsic = IEnum;
            type.name_id = parser->token->value.interner_id;
            parser_eat(parser, TOKEN_ID);
        } else if (parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_BOOL)) {
            parser_eat(parser, TOKEN_ID);
            type.intrinsic = INumeric;
            type.value = init_intrinsic_type(INumeric);
            type.value.numeric.type = NUMERIC_UNSIGNED;
            type.value.numeric.width = 1;
        } else if (is_template_type(parser->current_scope, parser->token->value.interner_id)) {
            type.intrinsic = ITemplate;
            type.value = init_intrinsic_type(ITemplate);
            type.name_id = parser->token->value.interner_id;
            parser_eat(parser, TOKEN_ID);
        } else {
            type.intrinsic = INumeric;
            type.value = init_intrinsic_type(INumeric);

            const char * type_str = interner_lookup_str(parser->token->value.interner_id)._ptr;
            println("type: {s}", type_str);

            switch (type_str[0]) {
            case 'i':
                type.value.numeric.type = NUMERIC_SIGNED; break;
            case 'u':
                type.value.numeric.type = NUMERIC_UNSIGNED; break;
            case 'f':
                type.value.numeric.type = NUMERIC_FLOAT; break;
            default:
                goto NUMERIC_PARSE_ERROR;
            }

            type.value.numeric.width = atoi(type_str + 1);

            if (type.value.numeric.width == 0) {
NUMERIC_PARSE_ERROR:
                FATAL("Invalid type: {s}", parser->token->value);
            }

            parser_eat(parser, TOKEN_ID);
        }

        break;
    case TOKEN_AMPERSAND:
        type.intrinsic = IRef;
        type.value = init_intrinsic_type(IRef);

        while (parser->token->type == TOKEN_AMPERSAND) {
            parser_eat(parser, TOKEN_AMPERSAND);
            type.value.ref.depth += 1;
        }

        ALLOC(type.value.ref.basetype);
        *type.value.ref.basetype = parser_parse_type(parser);

        break;
    case TOKEN_LBRACKET:
        parser_eat(parser, TOKEN_LBRACKET);
        type.intrinsic = IArray;
        type.value = init_intrinsic_type(IArray);

        switch (parser->token->type) {
        case TOKEN_INT:
            type.value.array.size = atoi(parser->token->value.span.start);
            parser_eat(parser, TOKEN_INT);
            break;
        case TOKEN_UNDERSCORE:
            FATAL("Slices are not yet implemented");
        default:
            type.value.array.size = -1;
        }

        parser_eat(parser, TOKEN_RBRACKET);

        ALLOC(type.value.array.basetype);
        *type.value.array.basetype = parser_parse_type(parser);

        break;
    case TOKEN_LPAREN:
        parser_eat(parser, TOKEN_LPAREN);
        type.intrinsic = ITuple;
        type.value = init_intrinsic_type(ITuple);

        do {
            if (parser->token->type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
            }

            ARENA_APPEND(&type.value.tuple.types, parser_parse_type(parser));
        } while (parser->token->type == TOKEN_COMMA);
        parser_eat(parser, TOKEN_RPAREN);

        break;
    default:
        FATAL("Invalid token type: {s}", token_type_to_str(parser->token->type));
    }

    return type;
}

union intrinsic_union init_intrinsic_type(enum intrinsic_type type) {
    union intrinsic_union value = {0};

    switch (type) {
        case INumeric: break;
        case IRef: break;
        case IArray: break;
        case IStruct:
        {
            value.structure.fields = init_list(sizeof(struct AST *));
        } break;
        case IEnum: break;
        case ITuple:
        {
            value.tuple.types = arena_init(sizeof(Type));
        } break;
        case IImpl: break;
        case ITemplate: break;
        case IVariable: break;
    }

    return value;
}

// char __is_template_type(struct hashmap * map, char * name) {
//     if (map == NULL)
//         return 0;
//
//     return hashmap_has(map, name);
// }

char is_template_type(struct AST * current_scope, unsigned int name_id) {
    struct AST * scope = get_scope(AST_FUNCTION, current_scope); 
    
    return 0;
    // return scope->type == AST_FUNCTION;
    //         && __is_template_type(scope->value.function.template_types, name);
}

struct AST * get_type(struct AST * ast, unsigned int name_id) {
    struct AST * scope = get_scope(AST_MODULE, ast->scope), * temp;
    a_module * module = &scope->value.module;
    a_struct * _struct;

    for (int i = 0; i < module->structures->size; ++i) {
        temp = list_at(module->structures, i);
        ASSERT1(temp->type == AST_SCOPE);
        _struct = &temp->value.structure;
        if (_struct->interner_id == name_id) {
            return temp;
        }
    }

    return NULL;
}

Type * ast_get_type_of(struct AST * ast) {
    switch (ast->type) {
        case AST_OP:
            {
                return ast->value.operator.type;
            }
        case AST_LITERAL:
            {
                return ast->value.literal.type;
            }
        case AST_VARIABLE:
            {
                return ast->value.variable.type;
            }
        default:
            FATAL("Unable to get a type from ast type '{s}'", ast_type_to_str(ast->type));
    }
}

Type ast_to_type(struct AST * ast) {
    Type type = {0};

    switch (ast->type) {
        case AST_EXPR:
        {
            a_expr expr = ast->value.expression;

            if (expr.children->size == 1) {
                Type * type_ref = ast_get_type_of(list_at(expr.children, 0));
                if (type_ref == NULL) {
                    return UNKNOWN_TYPE;
                }
                return *type_ref;
            }

            type.intrinsic = ITuple;
            type.value = init_intrinsic_type(ITuple);

            for (int i = 0; i < expr.children->size; ++i) {
                struct AST * child = list_at(expr.children, i);
                ASSERT1(child != NULL);
                DEBUG("Child: {s}", ast_to_string(child));

                Type * child_type = ast_get_type_of(child);
                ASSERT1(child_type != NULL);
                ARENA_APPEND(&type.value.tuple.types, *child_type);
            }
        } break;
        case AST_STRUCT:
        {
            a_struct _struct = ast->value.structure;

            type.name_id = _struct.interner_id;
            type.intrinsic = IStruct;
        } break;
        default:
        {
            FATAL("'{s}' is not an implemented type for type conversion", ast_type_to_str(ast->type));
        }
    }

    return type;
}

char is_implementer(Type implementation, Type implementer) {

}

char is_implicitly_equal(Type type1, Type type2) {
    if (!is_equal_type(get_base_type(type1), get_base_type(type2)))
        return 0;
 
    return 0;
}

char is_compatible_type(Type type1, Type type2) {
    if (type1.intrinsic == type2.intrinsic) {
        return is_equal_type(type1, type2);
    }

    if (type1.intrinsic == IImpl) {
        return is_implementer(type1, type2);
    } else if (type1.intrinsic == IImpl) {
        return is_implementer(type2, type1);
    }

    return 0;
}

char is_equal_type(Type type1, Type type2) {
    if (type1.intrinsic != type2.intrinsic) {
        return 0;
    }

    switch (type1.intrinsic) {
        case INumeric:
        {
            Numeric_T num1 = type1.value.numeric, num2 = type2.value.numeric;
            return num1.type == num2.type && num1.width == num2.width;
        }
        case IImpl:
        case IEnum:
        case IStruct:
        {
            return type1.name_id == type2.name_id;
        }
        case IArray:
        {
            Array_T arr1 = type1.value.array, arr2 = type2.value.array;
            ASSERT1(arr1.basetype != NULL);
            ASSERT1(arr2.basetype != NULL);
            return arr1.size == arr2.size && is_equal_type(*arr1.basetype, *arr2.basetype);
        }
        case IRef:
        {
            Ref_T ref1 = type1.value.ref, ref2 = type2.value.ref;
 
            ASSERT1(ref1.basetype != NULL);
            ASSERT1(ref2.basetype != NULL);
            return ref1.depth == ref2.depth && is_equal_type(*ref1.basetype, *ref2.basetype);
        }
        case ITuple:
        {
            Tuple_T tuple1 = type1.value.tuple,
                    tuple2 = type2.value.tuple;
            if (tuple1.types.size != tuple2.types.size) {
                return 0;
            }
            
            for (int i = 0; i < tuple1.types.size; ++i) {
                Type * child1 = arena_get(tuple1.types, i), * child2 = arena_get(tuple2.types, i);
                ASSERT1(child1 != NULL);
                ASSERT1(child2 != NULL);
                if (!is_equal_type(*child1, *child2)) {
                    return 0;
                }
            }
        } break;
        // default:
        // {
        //     FATAL("{u} is not an implemented intrinsic for checking type equivalance", type1.intrinsic);
        // }
    }

    return 1;
}

struct arena type_to_type_arena(Type type) {
    switch (type.intrinsic) {
        case ITuple:
            return type.value.tuple.types;
        default:
        {
            struct arena arena = arena_init(sizeof(Type));
            ARENA_APPEND(&arena, type);
            return arena;
        }
    }

}

Type get_base_type(Type type) {
    switch (type.intrinsic) {
        case IStruct:
        case INumeric:
        case ITemplate:
        case IImpl:
        case IEnum:
        case ITuple:
            return type;
        case IArray:
            return get_base_type(*type.value.array.basetype);
        case IRef:
            return get_base_type(*type.value.ref.basetype);
    }

    FATAL("Invalid type: {d}", type.intrinsic);
}

const char * get_base_type_str(Type type) {
    switch (type.intrinsic) {
        case IStruct:
        case INumeric:
        case ITemplate:
        case IImpl:
        case IEnum:
            return type_to_str(type);
        case IArray:
            return get_base_type_str(*type.value.array.basetype);
        case IRef:
            return get_base_type_str(*type.value.ref.basetype);
        case ITuple:
        {
            Tuple_T tuple = type.value.tuple;
            if (tuple.types.size == 1) {
                return get_base_type_str(*(Type *) arena_get(tuple.types, 0));
            }
            return NULL;
        }
    }

    return "(NULL)";
}

char * type_to_str(Type type) {
    switch (type.intrinsic) {
        case IUnknown:
            return "Unknown";
        case ITemplate:
            return interner_lookup_str(type.name_id)._ptr;
        case IStruct:
        case IEnum:
        {
            return interner_lookup_str(type.name_id)._ptr;
        }
        case INumeric:
        {
            Numeric_T num = type.value.numeric;
            char c = 'i';
            switch (num.type) {
                case NUMERIC_SIGNED:
                    break;
                case NUMERIC_UNSIGNED:
                    c = 'u'; break;
                case NUMERIC_FLOAT:
                    c = 'f'; break;
                default:
                    ERROR("Invalid numeric type: {i}", num.type);
            }
            return format("{c}{u}", c, num.width);
        }
        case IArray:
        {
            Array_T array = type.value.array;
            ASSERT(array.basetype != NULL, "Array basetype is invalidly NULL");
            return format("[]{s}", type_to_str(*array.basetype));
        }
        case IRef:
        {
            Ref_T ref = type.value.ref;
            char * buf = malloc(sizeof(char) * (ref.depth + 1));
            
            int i = 0;
            while (i < ref.depth) {
                buf[i++] = '&';
            }
            buf[i] = 0;

            ASSERT(ref.basetype != NULL, "Reference basetype is invalidly NULL");
            return format("{2s}", buf, type_to_str(*ref.basetype));
        }
        case ITuple:
        {
            Tuple_T tuple = type.value.tuple;

            if (tuple.types.size == 0) {
                return "()";
            }

            Type * temp = arena_get(tuple.types, 0);
            char * buf = format("({s}", type_to_str(*temp));

            for (int i = 1; i < tuple.types.size; ++i) {
                temp = arena_get(tuple.types, i);
                buf = format("{2s:, }", buf, type_to_str(*temp));
            }

            return format("{s})", buf);
        }
        case IImpl:
            return format("impl {s}", interner_lookup_str(type.name_id)._ptr);
        case IVariable:
            return format("?{u}", type.value.variable.ID);
    }

    return "(NULL)";
}

Type copy_type(Type src) {
    Type copy = src;

    copy.value = init_intrinsic_type(copy.intrinsic);
    switch (src.intrinsic) {
        case INumeric:
            copy.value.numeric = src.value.numeric; break;
        case IArray:
            copy.value.array = src.value.array; break;
        case IRef:
            copy.value.ref = src.value.ref; break;
        default:
            ASSERT(0, "Intrinsic type {i} is not implemented", copy.intrinsic);
    }
 
    return copy;
}

Type replace_self_in_type(Type to_replace, Type replacement) {
    Type type, prev, curr;

    char start = 1, finished = 0;

    while (!finished) {
        switch (to_replace.intrinsic) {
            case ITemplate:
                curr = copy_type(replacement);
                finished = 1;
                break;
            case IArray:
                curr = copy_type(to_replace);
                to_replace = *to_replace.value.array.basetype;
                break;
            case IRef:
                curr = copy_type(to_replace);
                to_replace = *to_replace.value.ref.basetype;
                break;
            default:
                ASSERT(0, "Invalid intrinsic type: {i}", to_replace.intrinsic);
        }

        if (start) {
            type = curr;
            start = 0;
        } else {
            switch (prev.intrinsic) {
                case IArray:
                    *prev.value.array.basetype = curr; break;
                case IRef:
                    *prev.value.ref.basetype = curr; break;
                default:
                    ASSERT(0, "Invalid intrinsic type");
            }
        }
        prev = curr;
    }

    return type;
}
