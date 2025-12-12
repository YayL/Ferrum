#include "parser/types.h"

#include "parser/AST.h"
#include "parser/keywords.h"
#include "parser/parser.h"
#include "tables/registry_manager.h"

struct registry_manager types_manager;

ID _parser_parse_numeric_type(struct Parser * parser) {
    enum Numeric_T_TYPE numeric_type;
    switch (parser->lexer.tok.span.start[0]) {
        case 'i': numeric_type = NUMERIC_SIGNED; break;
        case 'u': numeric_type = NUMERIC_UNSIGNED; break;
        case 'f': numeric_type = NUMERIC_FLOAT; break;
        default: return INVALID_ID;
    }

    SourceSpan span = parser->lexer.tok.span;
    char * end_ptr;
    long width = strtol(&span.start[1], &end_ptr, 10);

    if (!(0 < width && width <= UINT16_MAX) || end_ptr != &span.start[span.length]) {
        return INVALID_ID;
    }

    parser_eat(parser, TOKEN_ID);
    Numeric_T * numeric = type_allocate(ID_NUMERIC_TYPE);
    numeric->type = numeric_type;
    numeric->width = (unsigned short) width;

    return numeric->info.type_id;
}

ID parser_parse_type(struct Parser * parser) {
    switch (parser->lexer.tok.type) {
        case TOKEN_ID: {
            if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_BOOL))) {
                parser_eat(parser, TOKEN_ID);

                Numeric_T * numeric = type_allocate(ID_NUMERIC_TYPE);
                numeric->type = NUMERIC_UNSIGNED;
                numeric->width = 1;

                return numeric->info.type_id;
            } else if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_VOID))) {
                parser_eat(parser, TOKEN_ID);
                return VOID_TYPE;
            } else if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_PLACE))
                    || id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUTPLACE))) {
                char is_mut = id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUTPLACE));
                parser_eat(parser, TOKEN_ID);

                Place_T * place = type_allocate(ID_PLACE_TYPE);
                place->is_mut = is_mut;

                parser_eat(parser, TOKEN_LT);
                place->basetype_id = parser_parse_type(parser);
                parser_eat(parser, TOKEN_GT);

                return place->info.type_id;
            } else {
                ID numeric_type_id = _parser_parse_numeric_type(parser);

                if (!ID_IS_INVALID(numeric_type_id)) {
                    return numeric_type_id;
                }

                Symbol_T * type = type_allocate(ID_SYMBOL_TYPE);
                type->symbol_id = parser_parse_symbol(parser);
                type->templates = arena_init(sizeof(ID));

                // If there are no templates or empty template list
                if (parser->lexer.tok.type != TOKEN_LT || (parser_eat(parser, TOKEN_LT), parser->lexer.tok.type == TOKEN_GT && (parser_eat(parser, TOKEN_GT), 1))) {
                    return type->info.type_id;
                }

                do {
                    ARENA_APPEND(&type->templates, parser_parse_type(parser));
                } while (parser->lexer.tok.type == TOKEN_COMMA && (parser_eat(parser, TOKEN_COMMA), 1));
                parser_eat(parser, TOKEN_GT);

                return type->info.type_id;
            }

        } break;
        case TOKEN_AMPERSAND: {
            Ref_T * ref = type_allocate(ID_REF_TYPE);
            ref->is_mut = 0;
            ref->depth = 0;

            while (parser->lexer.tok.type == TOKEN_AMPERSAND) {
                parser_eat(parser, TOKEN_AMPERSAND);
                ref->depth += 1;
            }

            if (parser->lexer.tok.type == TOKEN_ID && id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUT))) {
                parser_eat(parser, TOKEN_ID);
                ref->is_mut = 1;
            }

            ref->basetype_id = parser_parse_type(parser);
            return ref->info.type_id;
        }
        case TOKEN_LBRACKET:
            parser_eat(parser, TOKEN_LBRACKET);

            Array_T * array = type_allocate(ID_ARRAY_TYPE);

            switch (parser->lexer.tok.type) {
                case TOKEN_INT:
                    array->size = atoi(parser->lexer.tok.span.start);
                    ASSERT1(array->size != -1);
                    parser_eat(parser, TOKEN_INT);
                    break;
                case TOKEN_UNDERSCORE:
                    parser_eat(parser, TOKEN_UNDERSCORE);
                    array->size = -1;
                    break;
                default:
                    FATAL("Slices are not yet implemented");
            }

            parser_eat(parser, TOKEN_RBRACKET);

            array->basetype_id = parser_parse_type(parser);

            return array->info.type_id;
        case TOKEN_LPAREN:
            parser_eat(parser, TOKEN_LPAREN);

            Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE);

            do {
                ARENA_APPEND(&tuple->types, parser_parse_type(parser));
            } while (parser->lexer.tok.type == TOKEN_COMMA && (parser_eat(parser, TOKEN_COMMA), 1));
            parser_eat(parser, TOKEN_RPAREN);

            return tuple->info.type_id;
        default:
            FATAL("Invalid token type: {s}", token_type_to_str(parser->lexer.tok.type));
    }
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

            Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE);

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
    Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE);

    tuple->types = arena;

    return tuple->info.type_id;
}

ID get_base_type(ID type_id) {
    switch (type_id.type) {
        case ID_SYMBOL_TYPE:
        case ID_NUMERIC_TYPE:
        case ID_TUPLE_TYPE:
            return type_id;
        case ID_ARRAY_TYPE:
            return get_base_type(LOOKUP(type_id, Array_T).basetype_id);
        case ID_REF_TYPE:
            return get_base_type(LOOKUP(type_id, Ref_T).basetype_id);
        default:
            FATAL("Invalid type: {s}", id_type_to_string(type_id.type));
    }

}

const char * get_base_type_str(ID type_id) {
    switch (type_id.type) {
        case ID_SYMBOL_TYPE:
        case ID_NUMERIC_TYPE:
        case ID_ARRAY_TYPE:
        case ID_REF_TYPE:
        case ID_TUPLE_TYPE:
            return type_to_str(get_base_type(type_id));
        default:
            return "(NULL)";
    }
}

#define RETURN_WITH_MUT_ADDED(IS_MUT, FMT, ...) if (IS_MUT) return format("mut " FMT, __VA_ARGS__); else return format(FMT, __VA_ARGS__);

const char * type_to_str(ID type_id) {
    switch (type_id.type) {
        case ID_INVALID_TYPE: {
            // WARN("Invalid type_id passed to type_to_str");
            return "?";
        }
        case ID_VOID_TYPE: {
            return "void";
        }
        case ID_SYMBOL_TYPE: {
            Symbol_T symbol_type = LOOKUP(type_id, Symbol_T);
            a_symbol symbol = LOOKUP(symbol_type.symbol_id, a_symbol);

            const char * str = interner_lookup_str(symbol.name_id)._ptr;

            if (symbol_type.templates.size > 0) {
                str = format("{s}<{s}", str, type_to_str(ARENA_GET(symbol_type.templates, 0, ID)));

                for (size_t i = 1; i < symbol_type.templates.size; ++i) {
                    str = format("{s}, {s}", str, type_to_str(ARENA_GET(symbol_type.templates, i, ID)));
                }
                str = format("{s}>", str);
            }

            return str;
        }
        case ID_NUMERIC_TYPE:
        {
            Numeric_T num = LOOKUP(type_id, Numeric_T);
            char c = 0;
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

            return format("{c}{u}", c, num.width);
        }
        case ID_ARRAY_TYPE:
        {
            Array_T array = LOOKUP(type_id, Array_T);
            ASSERT(!ID_IS_INVALID(array.basetype_id), "Array basetype is invalidly unknown");
            return format("[]{s}", type_to_str(array.basetype_id));
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

            if (ref.is_mut) {
                return format("{s}mut {s}", buf, type_to_str(ref.basetype_id));
            } else {
                return format("{2s}", buf, type_to_str(ref.basetype_id));
            }

        }
        case ID_TUPLE_TYPE: {
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
        case ID_PLACE_TYPE: {
            Place_T place = LOOKUP(type_id, Place_T);
            return format(place.is_mut 
                            ? "MutPlace<{s}>"
                            : "Place<{s}>"
                          , type_to_str(place.basetype_id));
        }
        case ID_FN_TYPE: {
            Fn_T fn = LOOKUP(type_id, Fn_T);
            return format("{s} -> {s}", type_to_str(fn.arg_type), type_to_str(fn.ret_type));
        }
        default: 
            FATAL("Unimplemented type_to_str type: {s}", id_type_to_string(type_id.type));
    }
}
