#include "parser/parser.h"

#include "common/io.h"
#include "parser/lexer.h"
#include "parser/modules.h"
#include "tables/interner.h"
#include "tables/registry_manager.h"
#include "tables/member_functions.h"

struct Parser init_parser(char * path) {
    return (struct Parser) {
        .lexer = lexer_init(path),
        .path = path,
        .error = 0,
        .previous_token = init_token(),
        .modules_to_parse = arena_init(sizeof(char *)),
    };
}

void parser_free(struct Parser parser) {
    arena_free(parser.modules_to_parse);
    lexer_free(&parser.lexer);
}

void parser_change_path(struct Parser * parser, char * path) {
    lexer_free(&parser->lexer);

    parser->lexer = lexer_init(path);

    parser->path = path;
    parser->previous_token = init_token();
}

void parser_eat(struct Parser * parser, enum token_t type) {
    if (parser->lexer.tok.type != type) {
        ERROR("{2i::} Expected token type '{s}' got token '{s}'", 
                parser->lexer.tok.line, parser->lexer.tok.pos, 
                token_type_to_str(type), 
                token_type_to_str(parser->lexer.tok.type));
        print_trace();
        parser->error = 1;
    }

    parser->previous_token = parser->lexer.tok;
    lexer_next_token(&parser->lexer);
}

void parser_eat_keyword(struct Parser * parser, enum Keywords keyword) {
    ID keyword_id = keyword_get_intern_id(keyword);

    if (parser->lexer.tok.type == TOKEN_ID && !id_is_equal(parser->lexer.tok.interner_id, keyword_id)){
        ERROR("{2i::} Expected keyword type '{s}' got token '{s}'", 
                parser->lexer.tok.line, parser->lexer.tok.pos, 
                interner_lookup_str(keyword_id)._ptr, 
                interner_lookup_str(parser->lexer.tok.interner_id)._ptr);
        parser->error = 1;
    }

    parser_eat(parser, TOKEN_ID);
}

Arena parser_parse_template_list(struct Parser * parser) {
    Arena arena = arena_init(sizeof(ID));

    if (parser->lexer.tok.type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);

        while (parser->lexer.tok.type == TOKEN_ID) {
            ID node_id = parser_parse_id(parser);
            ARENA_APPEND(&arena, node_id);

            if (parser->lexer.tok.type == TOKEN_GT) {
                break;
            }

            parser_eat(parser, TOKEN_COMMA);
        }

        parser_eat(parser, TOKEN_GT);
    }

    return arena;
}

ID parser_parse_int(struct Parser * parser) {
    a_literal * number = ast_allocate(ID_AST_LITERAL, parser->current_scope_id);

    Numeric_T * num = type_allocate(ID_NUMERIC_TYPE);
    num->width = 32;
    num->type = NUMERIC_SIGNED;
    number->type_id = num->info.type_id;

    number->literal_type = LITERAL_NUMBER;
    number->value = buffer_string_init_from_source_span(parser->lexer.tok.span);
    parser_eat(parser, TOKEN_INT);

    return number->info.node_id;
}

ID parser_parse_string(struct Parser * parser) {
    a_literal * string = ast_allocate(ID_AST_LITERAL, parser->current_scope_id);

    Array_T * array = type_allocate(ID_ARRAY_TYPE);
    Numeric_T * numeric = type_allocate(ID_NUMERIC_TYPE);
    numeric->type = NUMERIC_UNSIGNED;
    numeric->width = 8;

    array->size = parser->lexer.tok.span.length;
    array->basetype_id = numeric->info.type_id;
    string->type_id = array->info.type_id;

    string->literal_type = LITERAL_STRING;
    string->value = buffer_string_init_from_source_span(parser->lexer.tok.span);

    parser_eat(parser, TOKEN_STRING_LITERAL);

    return string->info.node_id;
}

ID parser_parse_id(struct Parser * parser) {
    a_symbol * symbol = ast_allocate(ID_AST_SYMBOL, parser->current_scope_id);

    const char * symbol_start = parser->lexer.tok.span.start;
    ID name_id = parser->lexer.tok.interner_id;
    ARENA_APPEND(&symbol->name_ids, name_id);

    parser_eat(parser, TOKEN_ID);

    // If only one colon
    if (parser->lexer.tok.type == TOKEN_COLON && (parser_eat(parser, TOKEN_COLON), parser->lexer.tok.type != TOKEN_COLON)) {
        a_variable * variable = ast_allocate(ID_AST_VARIABLE, parser->current_scope_id);
        variable->name_id = name_id;
        variable->type_id = parser_parse_type(parser);

        symbol->node_id = variable->info.node_id;
        symbol->name_id = name_id;

        return symbol->info.node_id;
    }

    // If more than one colon
    if (parser->lexer.tok.type ==  TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        ARENA_APPEND(&symbol->name_ids, parser->lexer.tok.interner_id);
        parser_eat(parser, TOKEN_ID);

        while (parser->lexer.tok.type == TOKEN_COLON) {
            parser_eat(parser, TOKEN_COLON);
            parser_eat(parser, TOKEN_COLON);

            ARENA_APPEND(&symbol->name_ids, parser->lexer.tok.interner_id);
            parser_eat(parser, TOKEN_ID);
        }

    }

    size_t symbols_length = parser->previous_token.span.start - symbol_start + parser->previous_token.span.length;
    symbol->name_id = interner_intern(source_span_init(symbol_start, symbols_length));

    // Try to parse templates
    if (parser->lexer.tok.type == TOKEN_LT) {
        struct Parser saved_parser = *parser;
        a_operator * op = ast_allocate(ID_AST_OP, parser->current_scope_id);
        Tuple_T * tuple = type_allocate(ID_TUPLE_TYPE);
        char restore_parser = 0;
        parser_eat(parser, TOKEN_LT);

        while (parser->lexer.tok.type != TOKEN_GT) {
            if (parser->lexer.tok.type != TOKEN_ID) {
                restore_parser = 1; break;
            }

            ID type_id = parser_parse_type(parser);
            if (ID_IS_INVALID(type_id)) {
                restore_parser = 1; break;
            }

            if (parser->lexer.tok.type == TOKEN_COMMA) {
                parser_eat(parser, TOKEN_COMMA);
                if (parser->lexer.tok.type == TOKEN_GT) {
                    restore_parser = 1; break;
                }
            } else if (parser->lexer.tok.type != TOKEN_GT) {
                restore_parser = 1; break;
            }

            ARENA_APPEND(&tuple->types, type_id);
        }

        if (!restore_parser) {
            parser_eat(parser, TOKEN_GT);
            // println("Found templates: {i}", tuple->types.size);
            // for (size_t i = 0; i < tuple->types.size; ++i) {
            //     println("\t({i}): {s}", i + 1, type_to_str(ARENA_GET(tuple->types, i, ID)));
            // }

            op->right_id = symbol->info.node_id;
            op->type_id = tuple->info.type_id;
            op->op = operator_get(TEMPLATE);

            return op->info.node_id;
        }

        *parser = saved_parser;
    }

    return symbol->info.node_id;
}

ID parser_parse_symbol(struct Parser * parser) {
    a_symbol * symbol = ast_allocate(ID_AST_SYMBOL, parser->current_scope_id);

    const char * symbol_start = parser->lexer.tok.span.start;
    ARENA_APPEND(&symbol->name_ids, parser->lexer.tok.interner_id);
    parser_eat(parser, TOKEN_ID);

    while (parser->lexer.tok.type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        parser_eat(parser, TOKEN_COLON);

        ARENA_APPEND(&symbol->name_ids, parser->lexer.tok.interner_id);
        parser_eat(parser, TOKEN_ID);
    }

    size_t symbols_length = parser->previous_token.span.start - symbol_start + parser->previous_token.span.length;
    symbol->name_id = interner_intern(source_span_init(symbol_start, symbols_length));

    return symbol->info.node_id;
}

ID parser_parse_if(struct Parser * parser) {
    a_if_statement * if_statement = ast_allocate(ID_AST_IF, parser->current_scope_id),
    * previous = NULL;

    ID if_node_id = if_statement->info.node_id;

    enum if_types {
        _NONE,
        _IF,
        _ELSE_IF,
        _ELSE
    } if_type = _IF;

    parser_eat_keyword(parser, KEYWORD_IF);

    ID keyword_if_id = keyword_get_intern_id(KEYWORD_IF);
    ID keyword_else_id = keyword_get_intern_id(KEYWORD_ELSE);

    do {
        while (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

        switch (if_type) {
            case _NONE: FATAL("If error?");
            case _IF:
            case _ELSE_IF:
                if_statement->expression_id = parser_parse_expr(parser);
            case _ELSE:
                if_statement->body_id = parser_parse_scope(parser);
        }

        if (!id_is_equal(parser->lexer.tok.interner_id, keyword_else_id)) {
            if_type = _NONE;
        } else {
            parser_eat_keyword(parser, KEYWORD_ELSE);
            if (id_is_equal(parser->lexer.tok.interner_id, keyword_if_id)) {
                if_type = _ELSE_IF;
                parser_eat_keyword(parser, KEYWORD_IF);
            } else {
                if_type = _ELSE;
            }

            previous = if_statement;
            if_statement = ast_allocate(ID_AST_IF, parser->current_scope_id);

            previous->next_id = if_statement->info.node_id;
        }

        ASSERT1(if_type != _IF);
    } while (if_type != _NONE);

    return if_node_id;
}

ID parser_parse_for(struct Parser * parser) {
    a_for_statement * for_statement = ast_allocate(ID_AST_FOR, parser->current_scope_id);

    parser_eat_keyword(parser, KEYWORD_FOR);

    for_statement->expression_id = parser_parse_expr(parser);
    for_statement->body_id = parser_parse_scope(parser);

    return for_statement->info.node_id;
}

ID parser_parse_while(struct Parser * parser) {
    a_while_statement * while_statement = ast_allocate(ID_AST_WHILE, parser->current_scope_id);

    parser_eat_keyword(parser, KEYWORD_WHILE);

    while_statement->expression_id = parser_parse_expr(parser);
    while_statement->body_id = parser_parse_scope(parser);

    return while_statement->info.node_id;
}

ID parser_parse_do(struct Parser * parser) {
    a_do_statement * do_statement = ast_allocate(ID_AST_DO, parser->current_scope_id);

    FATAL("Do statements are not implemented yet");

    return do_statement->info.node_id;
}

ID parser_parse_match(struct Parser * parser) {
    a_match_statement * match_statement = ast_allocate(ID_AST_MATCH, parser->current_scope_id);

    FATAL("Match statements are not implemented yet");

    return match_statement->info.node_id;
}

ID parser_parse_return(struct Parser * parser) {
    a_return_statement * return_statement = ast_allocate(ID_AST_RETURN, parser->current_scope_id);

    parser_eat_keyword(parser, KEYWORD_RETURN);
    return_statement->expression_id = parser_parse_expr(parser);

    return return_statement->info.node_id;
}

ID parser_parse_struct(struct Parser * parser) {
    a_structure * _struct = ast_allocate(ID_AST_STRUCT, parser->current_scope_id);
    parser->current_scope_id = _struct->info.node_id;

    parser_eat_keyword(parser, KEYWORD_STRUCT);
    _struct->generics = parser_parse_template_list(parser);

    _struct->name_id = parser->lexer.tok.interner_id;
    parser_eat(parser, TOKEN_ID); // [name]

    _struct->templates = parser_parse_template_list(parser);

    parser_eat(parser, TOKEN_LBRACE);

    // Add Self template to generics

    // a_symbol * self_template_symbol = ast_allocate(ID_AST_SYMBOL, parser->current_scope_id);
    // a_variable * self_template_var = ast_allocate(ID_AST_VARIABLE, parser->current_scope_id);
    // self_template_symbol->node_id = self_template_var->info.node_id;
    //
    // ID keyword_self_intern_id = keyword_get_intern_id(KEYWORD_SELF);
    // self_template_symbol->name_id = self_template_var->name_id = keyword_self_intern_id;
    // ARENA_APPEND(&self_template_symbol->name_ids, keyword_self_intern_id);
    //
    // Symbol_T * self_type = type_allocate(ID_SYMBOL_TYPE);
    // self_template_var->type_id = self_type->info.type_id;
    //
    // for (size_t i = 0; i < _struct->templates.size; ++i) {
    //     Symbol_T * template_symbol_type = type_allocate(ID_SYMBOL_TYPE);
    //     template_symbol_type->symbol_id = ARENA_GET(_struct->templates, i, ID);
    //     ARENA_APPEND(&self_type->templates, template_symbol_type->info.type_id);
    // }
    //
    // a_symbol * self_type_symbol = ast_allocate(ID_AST_SYMBOL, _struct->info.scope_id);
    // self_type_symbol->name_id = _struct->name_id;
    // self_type_symbol->node_id = _struct->info.node_id; // Immediatly connect it to this struct
    // ARENA_APPEND(&self_type_symbol->name_ids, _struct->name_id);
    //
    // self_type->symbol_id = self_type_symbol->info.node_id;
    //
    // println("type: {s}", type_to_str(self_type->info.type_id));
    // println("self_template: {s}", ast_to_string(self_template_symbol->info.node_id));
    // print_ast_tree(self_template_symbol->info.node_id);
    //
    // ARENA_APPEND(&_struct->generics, self_template_symbol->info.node_id);

    do {
        while (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

        ID child_node_id = parser_parse_identifier(parser);
        ASSERT1(!ID_IS_INVALID(child_node_id));

        switch (child_node_id.type) {
            case ID_AST_DECLARATION: {
                ARENA_APPEND(&_struct->declarations, child_node_id);
                a_expression expr = LOOKUP(LOOKUP(child_node_id, a_declaration).expression_id, a_expression);
                for (size_t i = 0; i < expr.children.size; ++i) {
                    ARENA_APPEND(&_struct->members, ARENA_GET(expr.children, i, ID));
                }
            } break;
            case ID_AST_FUNCTION:
                ARENA_APPEND(&_struct->members, child_node_id);
                ARENA_APPEND(&_struct->declarations, child_node_id);
                break;
            default: {
                FATAL("{2i::} Invalid identifier '{s}' in struct declaration", parser->lexer.line, parser->lexer.pos, id_type_to_string(child_node_id.type));
            }
        }

        if (parser->lexer.tok.type != TOKEN_RBRACE) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

    } while (parser->lexer.tok.type != TOKEN_RBRACE);

    println("Struct {s}", ast_to_string(_struct->info.node_id));

    parser_eat(parser, TOKEN_RBRACE);

    parser->current_scope_id = _struct->info.scope_id;
    return _struct->info.node_id;
}

ID parser_parse_trait(struct Parser * parser) {
    a_trait * trait = ast_allocate(ID_AST_TRAIT, parser->current_scope_id);
    parser->current_scope_id = trait->info.node_id;

    parser_eat_keyword(parser, KEYWORD_TRAIT); // trait
    trait->name_id = parser->lexer.tok.interner_id;
    parser_eat(parser, TOKEN_ID); // [name]
    trait->templates = parser_parse_template_list(parser);

    if (parser->lexer.tok.type == TOKEN_ID) {
        parser_eat_keyword(parser, KEYWORD_WHERE);

        do {
            ARENA_APPEND(&trait->where, parser_parse_type(parser));
        } while (parser->lexer.tok.type == TOKEN_COMMA && (parser_eat(parser, TOKEN_COMMA), 1));
    }

    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

        if (parser->lexer.tok.type == TOKEN_RBRACE) {
            break;
        }

        ID trait_member_id = parser_parse_identifier(parser);
        ASSERT1(!ID_IS_INVALID(trait_member_id));

        ARENA_APPEND(&trait->children, trait_member_id);
    }

    parser_eat(parser, TOKEN_RBRACE);

    parser->current_scope_id = trait->info.scope_id;
    return trait->info.node_id;
}

ID parser_parse_impl(struct Parser * parser) {
    a_implementation * impl = ast_allocate(ID_AST_IMPL, parser->current_scope_id);
    parser->current_scope_id = impl->info.node_id;

    parser_eat_keyword(parser, KEYWORD_IMPL);
    impl->generic_templates = parser_parse_template_list(parser);
    impl->trait_symbol_id = parser_parse_symbol(parser);
    impl->trait_templates = parser_parse_template_list(parser);

    if (parser->lexer.tok.type == TOKEN_ID) {
        parser_eat_keyword(parser, KEYWORD_WHERE);

        do {
            ARENA_APPEND(&impl->where, parser_parse_type(parser));
        } while (parser->lexer.tok.type == TOKEN_COMMA && (parser_eat(parser, TOKEN_COMMA), 1));
    }

    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

        if (parser->lexer.tok.type == TOKEN_RBRACE) {
            break;
        }

        ID id = parser_parse_identifier(parser);
        ASSERT1(!ID_IS_INVALID(id));
        ARENA_APPEND(&impl->members, id);
    }

    parser_eat(parser, TOKEN_RBRACE);

    parser->current_scope_id = impl->info.scope_id;

    return impl->info.node_id;
}

ID parser_parse_group(struct Parser * parser) {
    parser_eat_keyword(parser, KEYWORD_GROUP);
    a_group * group = ast_allocate(ID_AST_GROUP, parser->current_scope_id);

    group->name_id = parser->lexer.tok.interner_id;
    parser_eat(parser, TOKEN_ID);

    group->templates = parser_parse_template_list(parser);
    parser_eat_keyword(parser, KEYWORD_AS);

    group->type_id = parser_parse_type(parser);

    return group->info.node_id;
}

ID parser_parse_declaration(struct Parser * parser) {
    a_declaration * declaration = ast_allocate(ID_AST_DECLARATION, parser->current_scope_id);

    parser_eat_keyword(parser, KEYWORD_LET);
    declaration->is_mut = 0;

    if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUT))) {
        parser_eat_keyword(parser, KEYWORD_MUT);
        declaration->is_mut = 1;
    }

    declaration->expression_id = parser_parse_expr(parser);
    a_expression * expr = lookup(declaration->expression_id);

    ID child_node_id = ARENA_GET(expr->children, 0, ID);
    if (ID_IS(child_node_id, ID_AST_OP)) {
        a_operator op = LOOKUP(child_node_id, a_operator);
        if (op.op.key != ASSIGNMENT) {
            ERROR("Invalid declaration");
            exit(1);
        }

        child_node_id = op.left_id;
    }

    if (!ID_IS(child_node_id, ID_AST_SYMBOL)) {
        ERROR("{2i::} LHS of declaration must be a symbol", parser->lexer.tok.line, parser->lexer.tok.pos);
        print_trace();
        exit(1);
    }

    a_symbol * symbol = lookup(child_node_id);

    ASSERT1(symbol->name_ids.size != 0);
    if (symbol->name_ids.size > 1) {
        ERROR("Variable declaration does not allow names with '::'");
    }

    if (ID_IS_INVALID(symbol->node_id) && symbol->name_ids.size == 1) {
        a_variable * variable = ast_allocate(ID_AST_VARIABLE, parser->current_scope_id);
        variable->name_id = symbol->name_id;

        symbol->node_id = variable->info.node_id;
    }

    ASSERT1(ID_IS(symbol->node_id, ID_AST_VARIABLE));
    declaration->name_id = symbol->name_id;

    return declaration->info.node_id;
}

ID parser_parse_statement(struct Parser * parser) {
    if (parser->lexer.tok.type != TOKEN_ID || !keyword_interner_id_is_inbounds(parser->lexer.tok.interner_id)) {
        return parser_parse_expr(parser);
    }

    struct Keyword keyword = keyword_get_by_intern_id(parser->lexer.tok.interner_id);
    ASSERT(keyword.key != KEYWORD_NOT_FOUND, "Unable to find keyword '{s}'?", interner_lookup_str(parser->lexer.tok.interner_id)._ptr);

    switch (keyword.key) {
        case KEYWORD_LET:
            return parser_parse_declaration(parser);
        case KEYWORD_ELSE:
            FATAL("[Parser] Error else without if: {s}", token_to_str(parser->lexer.tok));
        case KEYWORD_IF:
            return parser_parse_if(parser);
        case KEYWORD_FOR:
            return parser_parse_for(parser);
        case KEYWORD_WHILE:
            return parser_parse_while(parser);
        case KEYWORD_DO:
            return parser_parse_do(parser);
        case KEYWORD_MATCH:
            return parser_parse_match(parser);
        case KEYWORD_RETURN:
            return parser_parse_return(parser);
        default:
            if (keyword.flag == GLOBAL_ONLY && keyword.flag != ANY) {
                FATAL("[Parser] {s} is not allowed in the function scope\n", token_to_str(parser->previous_token));
            }
    }

    FATAL("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
}

static inline void parser_parse_scope_statement(struct Parser * parser, a_scope * scope) {
    ID statement = parser_parse_statement(parser);
    ARENA_APPEND(&scope->nodes, statement);
    if (ID_IS(statement, ID_AST_DECLARATION)) {
        a_expression expr = LOOKUP(LOOKUP(statement, a_declaration).expression_id, a_expression);
        for (size_t i = 0; i < expr.children.size; ++i) {
            ARENA_APPEND(&scope->declarations, ARENA_GET(expr.children, i, ID));
        }
    }
}

ID parser_parse_scope(struct Parser * parser) {
    a_scope * scope = ast_allocate(ID_AST_SCOPE, parser->current_scope_id);
    parser->current_scope_id = scope->info.node_id;

    if (parser->lexer.tok.type == TOKEN_LBRACE) {
        parser_eat(parser, TOKEN_LBRACE);

        while (1) {
            while (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
                parser_eat(parser, TOKEN_LINE_BREAK);
            }

            if (parser->lexer.tok.type == TOKEN_RBRACE) {
                break;
            } else if (parser->lexer.tok.type == TOKEN_EOF) {
                FATAL("Unclosed scope");
            }

            parser_parse_scope_statement(parser, scope);
        }

        parser_eat(parser, TOKEN_RBRACE);
    } else {
        if (parser->lexer.tok.type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            WARN("Scopes without curly brackets must follow the scope initializer immidiatly(1 line or less)");
        } else {
            parser_parse_scope_statement(parser, scope);
        }
    }

    parser->current_scope_id = scope->info.scope_id;

    return scope->info.node_id;
}

ID parser_parse_function(struct Parser * parser) {
    a_function * function = ast_allocate(ID_AST_FUNCTION, parser->current_scope_id);
    parser->current_scope_id = function->info.node_id;

    parser_eat_keyword(parser, KEYWORD_FN); // fn
    
    if (id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_STATIC))) {
        function->is_static = 1;
        parser_eat_keyword(parser, KEYWORD_STATIC);
    }

    ID name_id = parser->lexer.tok.interner_id;
    parser_eat(parser, TOKEN_ID); // [name] or inline

    if (id_is_equal(name_id, keyword_get_intern_id(KEYWORD_INLINE))) {
        function->is_inline = 1;
        name_id = parser->lexer.tok.interner_id;
        parser_eat(parser, TOKEN_ID); // [name]
    }

    function->name_id = name_id;
    function->templates = parser_parse_template_list(parser);

    parser_eat(parser, TOKEN_LPAREN);

    function->arguments_id = parser_parse_expr_exit_on(parser, PARENTHESES);
    ASSERT1(ID_IS(function->arguments_id, ID_AST_EXPR));

    Fn_T * fn = type_allocate(ID_FN_TYPE);
    function->type = fn->info.type_id;
    fn->arg_type = ast_to_type(function->arguments_id);

    if (parser->lexer.tok.type == TOKEN_MINUS) {
        parser_eat(parser, TOKEN_MINUS);
        parser_eat(parser, TOKEN_GT);

        fn->ret_type = parser_parse_type(parser);
    } else {
        fn->ret_type = VOID_TYPE;
    }

    if (parser->lexer.tok.type == TOKEN_LBRACE) {
        function->body_id = parser_parse_scope(parser);
    } else {
        // parser_eat(parser, TOKEN_LINE_BREAK);
        function->body_id = INVALID_ID;
    }

    switch (function->info.scope_id.type) {
        case ID_AST_IMPL:
        case ID_AST_STRUCT:
            member_function_index_add(function->name_id, function->info.node_id);
        default: break;
    }

    parser->current_scope_id = function->info.scope_id;
    return function->info.node_id;
}

ID parser_parse_import(struct Parser * parser) {
    parser_eat_keyword(parser, KEYWORD_IMPORT);

    String current_path = string_init(parser->path);
    int last = current_path.length - 1; // offset of last '/' in path
    while (--last, last > 0 && current_path._ptr[last] != '/');
    ASSERT1(last != -1);
    current_path._ptr[last + 1] = '\0';

    char * module_path_cstr = source_span_to_cstr(parser->lexer.tok.span);

    char * formatted_path = format("{4s}", current_path._ptr, "/", module_path_cstr, ".fe");
    char * abs_path = get_abs_path(formatted_path);
    free_string(current_path);
    free(formatted_path);
    free(module_path_cstr);

    parser_eat(parser, TOKEN_STRING_LITERAL);

    parser_eat_keyword(parser, KEYWORD_AS);

    a_module * this_module = get_scope(ID_AST_MODULE, parser->current_scope_id);
    ASSERT(this_module != NULL, "Unable to find current module");
    ASSERT1(id_is_equal(this_module->info.node_id, parser->current_scope_id));

    ID imported_module_id = add_module(parser, abs_path);
    ASSERT1(!ID_IS_INVALID(imported_module_id));

    a_import * import = ast_allocate(ID_AST_IMPORT, parser->current_scope_id);
    import->name_id = parser->lexer.tok.interner_id;
    import->module_id = imported_module_id;

    parser_eat(parser, TOKEN_ID);

    return import->info.node_id;
}

ID parser_parse_identifier(struct Parser * parser) {
    ASSERT1(parser->lexer.tok.type == TOKEN_ID);
    struct Keyword identifier = keyword_get_by_intern_id(parser->lexer.tok.interner_id);

    if (identifier.flag != GLOBAL_ONLY && identifier.flag != ANY) {
        ERROR("[Parser] {s} is not allowed in the module scope", token_to_str(parser->lexer.tok));
        print_trace();
        exit(1);
    }

    switch (identifier.key) {
        case KEYWORD_NOT_FOUND:
            FATAL("[Parser]: {s} is not a valid identifier", token_to_str(parser->lexer.tok));
        case KEYWORD_FN:
            return parser_parse_function(parser);
        case KEYWORD_LET:
            return parser_parse_declaration(parser);
        case KEYWORD_IMPORT:
            return parser_parse_import(parser);
        case KEYWORD_STRUCT:
            return parser_parse_struct(parser);
        case KEYWORD_TRAIT:
            return parser_parse_trait(parser);
        case KEYWORD_IMPL:
            return parser_parse_impl(parser);
        case KEYWORD_GROUP:
            return parser_parse_group(parser);
        default:
            FATAL("Unknown identifier: '{s}'", keyword_get_str(identifier.key));
    }
}

void parser_parse_module(struct Parser * parser, a_module * module) {
    parser->current_scope_id = module->info.node_id;

    while (parser->lexer.tok.type != TOKEN_EOF) {
        if (parser->lexer.tok.type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
            continue;
        }

        ID module_member_id = parser_parse_identifier(parser);
        ASSERT1(!ID_IS_INVALID(module_member_id));
        ASSERT1(id_is_equal(parser->current_scope_id, module->info.node_id));

        ARENA_APPEND(&module->members, module_member_id);
        symbol_table_insert(&module->sym_table, module_member_id);
    }
}

void parser_parse(a_root * root, char * path) {
    struct Parser parser = init_parser(path);
    parser.root = root;
    parser.current_scope_id = root->info.node_id;

    root->entry_point = path;

    add_module(&parser, path);

    while (parser.modules_to_parse.size > 0) {
        path = ARENA_POP(&parser.modules_to_parse, char *);
        ID module_id = find_module(root, path);
        ASSERT1(!ID_IS_INVALID(module_id));

        INFO("Parsing module '{s}'", path);
        parser_change_path(&parser, path);
        parser_parse_module(&parser, lookup(module_id));
    }

    parser_free(parser);
}
