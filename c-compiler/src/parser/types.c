#include "parser/types.h"

#include "parser/AST.h"
#include "parser/keywords.h"
#include "parser/parser.h"
#include "tables/registry_manager.h"

struct registry_manager types_manager;

ID _parser_parse_numeric_type(struct Parser * parser, char is_mut) {
    enum Numeric_T_TYPE numeric_type;
    switch (parser->lexer.tok.span.start[0]) {
        case 'i': numeric_type = NUMERIC_SIGNED; break;
        case 'u': numeric_type = NUMERIC_UNSIGNED; break;
        case 'f': numeric_type = NUMERIC_FLOAT; break;
        default: return INVALID_ID;
    }

    SourceSpan span = parser->lexer.tok.span;
    char * end_ptr;
    int width = strtol(&span.start[1], &end_ptr, 10);

    if (!(0 < width && width <= UINT16_MAX) || end_ptr != &span.start[span.length]) {
        println("invalid: {u}, {u}", end_ptr - span.start, span.length);
        return INVALID_ID;
    }

    parser_eat(parser, TOKEN_ID);
    Numeric_T * numeric = type_allocate(ID_NUMERIC_TYPE, is_mut);
    numeric->type = numeric_type;
    numeric->width = (unsigned short) width;
    return numeric->info.type_id;
}

ID parser_parse_type(struct Parser * parser) {
    char is_mut = 0;

    if (parser->lexer.tok.type == TOKEN_ID && id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUT))) {
        parser_eat_keyword(parser, KEYWORD_MUT);
        is_mut = 1;
    }

    switch (parser->lexer.tok.type) {
    case TOKEN_ID:
        if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_IMPL))) {
            parser_eat(parser, TOKEN_ID);

            Impl_T * impl = type_allocate(ID_IMPL_TYPE, is_mut);
            impl->symbol_id = parser_parse_symbol(parser);

            return impl->info.type_id;
        } else if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_BOOL))) {
            parser_eat(parser, TOKEN_ID);

            Numeric_T * numeric = type_allocate(ID_NUMERIC_TYPE, is_mut);
            numeric->type = NUMERIC_UNSIGNED;
            numeric->width = 1;
            
            return numeric->info.type_id;
        } else {
            ID numeric_type_id = _parser_parse_numeric_type(parser, is_mut);

            if (!ID_IS_INVALID(numeric_type_id)) {
                return numeric_type_id;
            }

            Symbol_T * type = type_allocate(ID_SYMBOL_TYPE, is_mut);
            type->symbol_id = parser_parse_symbol(parser);

            if (parser->lexer.tok.type == TOKEN_LT) {
                FATAL("Template type parsing is not implemented yet");
            }

            return type->info.type_id;
        }

        break;
    case TOKEN_AMPERSAND: {
        Ref_T * ref = type_allocate(ID_REF_TYPE, is_mut);
        ref->depth = 0;

        while (parser->lexer.tok.type == TOKEN_AMPERSAND) {
            parser_eat(parser, TOKEN_AMPERSAND);
            ref->depth += 1;
        }

        ref->basetype_id = parser_parse_type(parser);
        return ref->info.type_id;
    }
    case TOKEN_LBRACKET:
        parser_eat(parser, TOKEN_LBRACKET);

        Array_T * array = type_allocate(ID_ARRAY_TYPE, is_mut);

        switch (parser->lexer.tok.type) {
        case TOKEN_INT:
            array->size = atoi(parser->lexer.tok.span.start);
            parser_eat(parser, TOKEN_INT);
            break;
        case TOKEN_UNDERSCORE:
            FATAL("Slices are not yet implemented");
        default:
            array->size = -1;
        }

        parser_eat(parser, TOKEN_RBRACKET);

        array->basetype_id = parser_parse_type(parser);

        return array->info.type_id;
    case TOKEN_LPAREN:
        parser_eat(parser, TOKEN_LPAREN);

        Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE, is_mut);

        do {
            ARENA_APPEND(&tuple->types, parser_parse_type(parser));
        } while (parser->lexer.tok.type == TOKEN_COMMA && (parser_eat(parser, TOKEN_COMMA), 1));
        parser_eat(parser, TOKEN_RPAREN);

        return tuple->info.type_id;
    default:
        FATAL("Invalid token type: {s}", token_type_to_str(parser->lexer.tok.type));
    }
}

void type_init_intrinsic_type(enum id_type type, void * type_ref) {
    switch (type) {
        case ID_TUPLE_TYPE:
            ((Tuple_T *) type_ref)->types = arena_init(sizeof(ID)); break;
        case ID_NUMERIC_TYPE:
        case ID_SYMBOL_TYPE:
        case ID_IMPL_TYPE:
        case ID_ARRAY_TYPE:
        case ID_REF_TYPE:
            break;
        default:
            println("Invalid ID type: {s}", id_type_to_string(type));
    }
}

// char __is_template_type(struct hashmap * map, char * name) {
//     if (map == NULL)
//         return 0;
//
//     return hashmap_has(map, name);
// }

char is_template_type(ID scope_id, ID name_id) {
    struct AST * scope = get_scope(ID_AST_FUNCTION, scope_id); 
    
    return 0;
    // return scope->type == AST_FUNCTION;
    //         && __is_template_type(scope->value.function.template_types, name);
}

ID ast_get_type_of(ID node_id) {
    switch (node_id.type) {
        case ID_AST_OP:
            {
                a_operator operator = LOOKUP(node_id, a_operator);
                ASSERT1(!ID_IS_INVALID(operator.type_id));
                return operator.type_id;
            }
        case ID_AST_LITERAL:
            {
                a_literal literal = LOOKUP(node_id, a_literal);
                ASSERT1(!ID_IS_INVALID(literal.type_id));
                return literal.type_id;
            }
        case ID_AST_VARIABLE:
            {
                a_variable variable = LOOKUP(node_id, a_variable);
                ASSERT1(!ID_IS_INVALID(variable.type_id));
                return variable.type_id;
            }
        case ID_AST_SYMBOL:
            {
                a_symbol symbol = LOOKUP(node_id, a_symbol);
                ASSERT1(!ID_IS_INVALID(symbol.node_id));
                return ast_get_type_of(symbol.node_id);
            }
        default:
            FATAL("Unable to get a type from ast type '{s}'", id_type_to_string(node_id.type));
    }
}

ID ast_to_type(ID node_id) {
    switch (node_id.type) {
        case ID_AST_EXPR:
        {
            a_expression expr = LOOKUP(node_id, a_expression);

            Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE, 0);

            for (int i = 0; i < expr.children.size; ++i) {
                ID child_node_id = ARENA_GET(expr.children, i, ID);
                ASSERT1(!ID_IS_INVALID(child_node_id));

                ARENA_APPEND(&tuple->types, ast_get_type_of(child_node_id));
            }

            return tuple->info.type_id;
        } break;
        default:
        {
            FATAL("'{s}' is not an implemented type for type conversion", id_type_to_string(node_id.type));
        }
    }
}

Arena type_to_type_arena(ID type_id) {
    switch (type_id.type) {
        case ID_TUPLE_TYPE:
            return LOOKUP(type_id, Tuple_T).types;
        default:
        {
            Arena arena = arena_init(sizeof(ID));
            ARENA_APPEND(&arena, type_id);
            return arena;
        }
    }
}

ID type_from_arena(Arena arena) {
    Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE, 0);

    tuple->types = arena;

    return tuple->info.type_id;
}

ID get_base_type(ID type_id) {
    switch (type_id.type) {
        case ID_SYMBOL_TYPE:
        case ID_NUMERIC_TYPE:
        case ID_IMPL_TYPE:
        case ID_TUPLE_TYPE:
            return type_id;
        case ID_ARRAY_TYPE:
            return get_base_type(LOOKUP(type_id, Array_T).basetype_id);
        case ID_REF_TYPE:
            return get_base_type(LOOKUP(type_id, Ref_T).basetype_id);
    }

    FATAL("Invalid type: {s}", id_type_to_string(type_id.type));
}

const char * get_base_type_str(ID type_id) {
    switch (type_id.type) {
        case ID_SYMBOL_TYPE:
        case ID_NUMERIC_TYPE:
        case ID_IMPL_TYPE:
        case ID_ARRAY_TYPE:
        case ID_REF_TYPE:
        case ID_TUPLE_TYPE:
            return type_to_str(get_base_type(type_id));
    }

    return "(NULL)";
}

#define RETURN_WITH_MUT_ADDED(IS_MUT, FMT, ...) if (IS_MUT) return format("mut " FMT, __VA_ARGS__); else return format(FMT, __VA_ARGS__);

char * type_to_str(ID type_id) {
    switch (type_id.type) {
        case ID_SYMBOL_TYPE:
        {
            Symbol_T symbol_type = LOOKUP(type_id, Symbol_T);
            a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);
            if (symbol_type.info.is_mut) {
                return format("mut {s}", interner_lookup_str(symbol.name_id)._ptr);
            } else {
                return interner_lookup_str(symbol.name_id)._ptr;
            }
        }
        case ID_NUMERIC_TYPE:
        {
            Numeric_T num = LOOKUP(type_id, Numeric_T);
            char c;
            switch (num.type) {
                case NUMERIC_SIGNED:
                    c = 'i'; break;
                case NUMERIC_UNSIGNED:
                    c = 'u'; break;
                case NUMERIC_FLOAT:
                    c = 'f'; break;
                default:
                    ERROR("Invalid numeric type: {i}", num.type);
            }

            RETURN_WITH_MUT_ADDED(num.info.is_mut, "{c}{u}", c, num.width);
        }
        case ID_ARRAY_TYPE:
        {
            Array_T array = LOOKUP(type_id, Array_T);
            ASSERT(!ID_IS_INVALID(array.basetype_id), "Array basetype is invalidly unknown");
            RETURN_WITH_MUT_ADDED(array.info.is_mut, "[]{s}", type_to_str(array.basetype_id));
        }
        case ID_REF_TYPE:
        {
            Ref_T ref = LOOKUP(type_id, Ref_T);
            char * buf = malloc(sizeof(char) * (ref.depth + 1));
            
            int i = 0;
            while (i < ref.depth) {
                buf[i++] = '&';
            }
            buf[i] = 0;

            ASSERT(!ID_IS_INVALID(ref.basetype_id), "Reference basetype is invalidly unknown");
            RETURN_WITH_MUT_ADDED(ref.info.is_mut, "{2s}", buf, type_to_str(ref.basetype_id));
        }
        case ID_TUPLE_TYPE:
        {
            Tuple_T tuple = LOOKUP(type_id, Tuple_T);

            if (tuple.types.size == 0) {
                return "()";
            }

            ID child_type_id = ARENA_GET(tuple.types, 0, ID);
            char * buf = format("({s}", type_to_str(child_type_id));

            for (int i = 1; i < tuple.types.size; ++i) {
                child_type_id = ARENA_GET(tuple.types, i, ID);
                buf = format("{2s:, }", buf, type_to_str(child_type_id));
            }

            return format("{s})", buf);
        }
        case ID_IMPL_TYPE: {
            a_symbol symbol = LOOKUP(LOOKUP(type_id, Impl_T).symbol_id, a_symbol);
            return format("impl {s}", interner_lookup_str(symbol.name_id)._ptr);
        }
    }

    return "(NULL)";
}
