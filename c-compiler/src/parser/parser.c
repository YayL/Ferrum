#include "parser/parser.h"

#include "codegen/AST.h"
#include "common/arena.h"
#include "common/hashmap.h"
#include "common/io.h"
#include "common/logger.h"
#include "common/string.h"
#include "fmt.h"
#include "parser/keywords.h"
#include "parser/modules.h"
#include "parser/lexer.h"
#include "common/macro.h"
#include "parser/token.h"
#include <stdlib.h>

struct Parser * init_parser(char * path) {
    struct Parser * parser = malloc(sizeof(struct Parser));

    size_t length;
    char * src = read_file(path, &length);
    struct Lexer * lexer = lexer_init(src, length);

    parser->path = path;
    parser->lexer = lexer;
    parser->error = 0;
    parser->prev = init_token();

    parser->modules_to_parse = arena_init(sizeof(char *));

    lexer_next_token(lexer);
    parser->token = &lexer->tok;
   
    return parser;
}

void parser_change_path(struct Parser * parser, char * path) {
    lexer_free(parser->lexer);

    size_t length;
    char * src = read_file(path, &length);
    parser->lexer = lexer_init(src, length);

    parser->path = path;
    parser->prev = init_token();

    lexer_next_token(parser->lexer);
    parser->token = &parser->lexer->tok;
}

void parser_eat(struct Parser * parser, enum token_t type) {
    if (parser->token->type != type) {
        println("[Error] {2i::} Expected token type '{s}' got token '{s}'", 
                    parser->token->line, parser->token->pos, 
                    token_type_to_str(type), 
                    token_type_to_str(parser->token->type));
        print_trace();
        parser->error = 1;
    }

    if (parser->prev.type != TOKEN_EOF && parser->token != NULL) {
        parser->prev = *parser->token;
    }

    lexer_next_token(parser->lexer);
}

void parser_eat_keyword(struct Parser * parser, enum Keywords keyword) {
    unsigned int keyword_id = keyword_get_intern_id(keyword);

    if (parser->token->type == TOKEN_ID && parser->token->interner_id != keyword_id){
        println("[Error] {2i::} Expected keyword type '{s}' got token '{s}'", 
                    parser->token->line, parser->token->pos, 
                    interner_lookup_str(keyword_id)._ptr, 
                    interner_lookup_str(parser->token->interner_id)._ptr);
        parser->error = 1;
    }

    parser_eat(parser, TOKEN_ID);
}

Arena parser_parse_template_list(struct Parser * parser) {
    Arena arena = arena_init(sizeof(struct AST *));

    if (parser->token->type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);

        while (parser->token->type == TOKEN_ID) {
            struct AST * ast = parser_parse_id(parser);
            ARENA_APPEND(&arena, ast);

            if (parser->token->type == TOKEN_GT) {
                break;
            }

            parser_eat(parser, TOKEN_COMMA);
        }

        parser_eat(parser, TOKEN_GT);
    }

    return arena;
}

struct AST * parser_parse_int(struct Parser * parser) {
    struct AST * ast = init_ast(AST_LITERAL, parser->current_scope);
    a_literal * number = &ast->value.literal;

    Numeric_T num = init_intrinsic_type(INumeric).numeric;
    num.type = NUMERIC_SIGNED;
    num.width = 32;

    ALLOC(number->type);
    Type * type = number->type;

    type->value.numeric = num;
    type->intrinsic = INumeric;

    number->value = parser->token->span;

    parser_eat(parser, TOKEN_INT);

    return ast;
}

struct AST * parser_parse_string(struct Parser * parser) {
    struct AST * ast = init_ast(AST_LITERAL, parser->current_scope);
    a_literal * string = &ast->value.literal;

    ALLOC(string->type);
    Type * type = string->type;

    Array_T array = init_intrinsic_type(IArray).array;
    array.size = parser->token->span.length;
    array.basetype = malloc(sizeof(Type));
    
    Numeric_T num = init_intrinsic_type(INumeric).numeric;
    num.type = NUMERIC_UNSIGNED;
    num.width = 8;

    array.basetype->value.numeric = num;
    array.basetype->intrinsic = INumeric;

    type->value.array = array;
    type->intrinsic = IArray;

    string->literal_type = LITERAL_STRING;
    string->value = parser->token->span;

    parser_eat(parser, TOKEN_STRING_LITERAL);

    return ast;
}

struct AST * parser_parse_id(struct Parser * parser) {
    struct AST * ast = init_ast(AST_SYMBOL, parser->current_scope);
    a_symbol * symbol = &ast->value.symbol;
    unsigned int name_id = parser->token->interner_id;
    ARENA_APPEND(&symbol->name_ids, name_id);
    parser_eat(parser, TOKEN_ID);

     if (parser->token->type == TOKEN_COLON && (parser_eat(parser, TOKEN_COLON), parser->token->type != TOKEN_COLON)) {
        struct AST * var_ast = init_ast(AST_VARIABLE, parser->current_scope);
        a_variable * variable = &var_ast->value.variable;
        variable->name_id = name_id;
        variable->type = parser_parse_type(parser);

        ast->value.symbol.node = var_ast;
    } else if (parser->token->type ==  TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        ARENA_APPEND(&symbol->name_ids, parser->token->interner_id);
        parser_eat(parser, TOKEN_ID);

        while (parser->token->type == TOKEN_COLON) {
            parser_eat(parser, TOKEN_COLON);
            parser_eat(parser, TOKEN_COLON);

            ARENA_APPEND(&symbol->name_ids, parser->token->interner_id);
            parser_eat(parser, TOKEN_ID);
        }
    }

    return ast;
}

struct AST * parser_parse_symbol(struct Parser * parser) {
    struct AST * ast = init_ast(AST_SYMBOL, parser->current_scope);
    a_symbol * symbol = &ast->value.symbol;

    ARENA_APPEND(&symbol->name_ids, parser->token->interner_id);
    parser_eat(parser, TOKEN_ID);

    while (parser->token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        parser_eat(parser, TOKEN_COLON);

        ARENA_APPEND(&symbol->name_ids, parser->token->interner_id);
        parser_eat(parser, TOKEN_ID);
    }

    return ast;
}

struct AST * parser_parse_if(struct Parser * parser) {
    struct AST * ast = init_ast(AST_IF, parser->current_scope);
    
    a_if_statement * if_statement = NULL,
                   * previous;
    char is_else = 0;
    
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        is_else = 0;

        if (if_statement && parser->token->type == TOKEN_ID && parser->token->interner_id == keyword_get_intern_id(KEYWORD_ELSE)) {
            parser_eat(parser, TOKEN_ID);
            is_else = 1;
        }

        if (if_statement) {
            if_statement->next = malloc(sizeof(a_if_statement));
            *if_statement->next = init_ast_value(AST_IF).if_statement;

            previous = if_statement;
            if_statement = if_statement->next;
        } else {
            if_statement = &ast->value.if_statement;
        }

        if (parser->token->type == TOKEN_ID && parser->token->interner_id == keyword_get_intern_id(KEYWORD_IF)) {
            parser_eat(parser, TOKEN_ID);
            if_statement->expression = parser_parse_expr(parser);
            if_statement->body = parser_parse_scope(parser);
        } else {
            if (is_else) {
                if_statement->body = parser_parse_scope(parser);
            } else {
                previous->next = NULL;
                free(if_statement);
            }
            break;
        }
    }

    return ast;
}

struct AST * parser_parse_for(struct Parser * parser) {
    struct AST * ast = init_ast(AST_FOR, parser->current_scope);
    a_for_statement * for_statement = &ast->value.for_statement;

    struct Token * for_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    
    for_statement->expression = parser_parse_expr(parser);
    for_statement->body = parser_parse_scope(parser);

    return ast;
}

struct AST * parser_parse_while(struct Parser * parser) {
    struct Token * while_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    struct AST * ast = init_ast(AST_WHILE, parser->current_scope);
    a_while_statement * while_statement = &ast->value.while_statement;
    
    while_statement->expression = parser_parse_expr(parser);
     
    while_statement->body = parser_parse_scope(parser);

    return ast;
}

struct AST * parser_parse_do(struct Parser * parser) {
    struct AST * ast = init_ast(AST_DO, parser->current_scope);

    FATAL("Do statements are not implemented yet");

    return ast;
}

struct AST * parser_parse_match(struct Parser * parser) {
    struct AST * ast = init_ast(AST_MATCH, parser->current_scope);

    FATAL("Match statements are not implemented yet");

    return ast;
}

struct AST * parser_parse_return(struct Parser * parser) {
    struct AST * ast = init_ast(AST_RETURN, parser->current_scope);
    a_return_statement * return_statement = &ast->value.return_statement;

    parser_eat(parser, TOKEN_ID);    
    return_statement->expression = parser_parse_expr(parser);

    return ast;
}

void parser_parse_template_types(struct Parser * parser, struct hashmap * map) { 
}

struct AST * parser_parse_struct(struct Parser * parser) {
    struct AST * ast = init_ast(AST_STRUCT, parser->current_scope), * temp;
    a_structure * _struct = &ast->value.structure;
    
    parser_eat(parser, TOKEN_ID); // struct
    _struct->name_id = parser->token->interner_id;
    parser_eat(parser, TOKEN_ID); // [name]

    _struct->templates = parser_parse_template_list(parser);

    parser_eat(parser, TOKEN_LBRACE);

    do {
        temp = parser_parse_identifier(parser);
        if (temp != NULL) {
            switch (temp->type) {
                case AST_DECLARATION:
                    {
                        a_expression expr = temp->value.declaration.expression->value.expression;
                        arena_extend(&_struct->definitions, expr.children);
                    } break;
                case AST_FUNCTION:
                    {
                        ARENA_APPEND(&_struct->definitions, temp);
                    } break;
                default:
                    {
                        FATAL("{2i::} Invalid identifier '{s}' in struct declaration", parser->lexer->line, parser->lexer->pos, ast_type_to_str(temp->type));
                    }
            }
        }

        parser_eat(parser, TOKEN_LINE_BREAK);
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

    } while (parser->token->type != TOKEN_RBRACE);

    parser_eat(parser, TOKEN_RBRACE);

    return ast;
}

struct AST * parser_parse_trait(struct Parser * parser) {
    struct AST * ast = init_ast(AST_TRAIT, parser->current_scope),
               * node;
    
    parser->current_scope = ast;

    a_trait * trait = &ast->value.trait;
    parser_eat(parser, TOKEN_ID); // trait
    trait->name_id = parser->token->interner_id;
    parser_eat(parser, TOKEN_ID); // [name]

    parser_eat(parser, TOKEN_LBRACE);
    
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_RBRACE)
            break;
        
        node = parser_parse_identifier(parser);
        if (node != NULL) {
            ARENA_APPEND(&trait->children, node);
        }
    }

    parser_eat(parser, TOKEN_RBRACE);
    
    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_impl(struct Parser * parser) {
    struct AST * ast = init_ast(AST_IMPL, parser->current_scope);
    a_implementation * impl = &ast->value.implementation;
    parser->current_scope = ast;

    parser_eat(parser, TOKEN_ID); // impl
    impl->name_id = parser->token->interner_id;
    parser_eat(parser, TOKEN_ID); // [name]
    parser_eat(parser, TOKEN_ID); // for

    impl->type = parser_parse_type(parser);

    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
        }

        if (parser->token->type == TOKEN_RBRACE) {
            break;
        }

        struct AST ** next = arena_next(&impl->members);
        // println("{p}", next);
        *next = parser_parse_identifier(parser);
        struct AST * node = ARENA_GET(impl->members, impl->members.size - 1, struct AST *);
        // println("inserted: {s}", ast_to_string(node));
        // ARENA_APPEND(&impl->members, parser_parse_identifier(parser));
    }

    parser_eat(parser, TOKEN_RBRACE);

    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_declaration(struct Parser * parser, enum Keywords keyword) {
    struct AST * ast = init_ast(AST_DECLARATION, parser->current_scope),
               * node;

    a_declaration * declaration = &ast->value.declaration;
    declaration->is_const = keyword == KEYWORD_CONST;

    parser_eat(parser, TOKEN_ID);

    declaration->expression = parser_parse_expr(parser);
    a_expression * expr = &declaration->expression->value.expression;

    node = ARENA_GET(expr->children, 0, struct AST *);
    if (node->type == AST_OP) {
        a_operator op = node->value.operator;
        node = op.left;
    }

    if (node->type != AST_SYMBOL) {
        FATAL("{2i::} LHS of declaration must be a symbol", parser->token->line, parser->token->pos);
    }

    a_symbol * symbol = &node->value.symbol;
    symbol->node = ast;

    ASSERT1(symbol->name_ids.size != 0);
    if (symbol->name_ids.size > 1) {
        ERROR("Variable declaration does not allow names with '::'");
    }

    declaration->name_id = ARENA_GET(symbol->name_ids, 0, unsigned int);

    return ast;
}

struct AST * parser_parse_statement(struct Parser * parser) {
    if (parser->token->type != TOKEN_ID || !keyword_interner_id_is_inbounds(parser->token->interner_id)) {
        return parser_parse_expr(parser);
    }

    struct Keyword keyword = keyword_get_by_intern_id(parser->token->interner_id);
    ASSERT(keyword.key != KEYWORD_NOT_FOUND, "Unable to find keyword '{s}'?", interner_lookup_str(parser->token->interner_id)._ptr);

    switch (keyword.key) {
        case KEYWORD_CONST:
        case KEYWORD_LET:
            return parser_parse_declaration(parser, keyword.key);
        case KEYWORD_ELSE:
            FATAL("[Parser] Error else without if: {s}", token_to_str(*parser->token));
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
                FATAL("[Parser] {s} is not allowed in the function scope\n", token_to_str(parser->prev));
            }
    }

    FATAL("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
}

struct AST * parser_parse_scope(struct Parser * parser) {
    struct AST * ast = init_ast(AST_SCOPE, parser->current_scope), * statement;
    a_scope * scope = &ast->value.scope;
    parser->current_scope = ast;
    
    if (parser->token->type == TOKEN_LBRACE) {
        parser_eat(parser, TOKEN_LBRACE);

        while (1) {
            while (parser->token->type == TOKEN_LINE_BREAK) {
                parser_eat(parser, TOKEN_LINE_BREAK);
            }

            if (parser->token->type == TOKEN_RBRACE) {
                break;
            } else if (parser->token->type == TOKEN_EOF) {
                FATAL("Unclosed scope");
            }

            ARENA_APPEND(&scope->nodes, parser_parse_statement(parser));
        }

        parser_eat(parser, TOKEN_RBRACE);
    } else {
        if (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_LINE_BREAK) {
            WARN("Scopes without curly brackets must follow the scope initializer immidiatly(1 line or less)");
        } else {
            ARENA_APPEND(&scope->nodes, parser_parse_statement(parser));
        }
    }

    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_function(struct Parser * parser) {
    struct AST * ast = init_ast(AST_FUNCTION, parser->current_scope);
    a_function * function = &ast->value.function;
    parser->current_scope = ast;

    parser_eat(parser, TOKEN_ID); // fn

    // inline
    if (parser->token->type == TOKEN_ID && parser->token->interner_id == keyword_get_intern_id(KEYWORD_INLINE)) {
        function->is_inline = 1;
        parser_eat(parser, TOKEN_ID);
    }

    // [name]
    if (parser->token->type == TOKEN_ID) {
        function->name_id = parser->token->interner_id;
        parser_eat(parser, TOKEN_ID);
    }
    
    function->templates = parser_parse_template_list(parser);

    parser_eat(parser, TOKEN_LPAREN);
    
    function->arguments = parser_parse_expr_exit_on(parser, PARENTHESES);
    ASSERT1(function->arguments->type == AST_EXPR);

    if (function->arguments->value.expression.children.size != 0) {
        ALLOC(function->param_type);
        *function->param_type = ast_to_type(function->arguments);
    }
    
    if (parser->token->type == TOKEN_MINUS) {
        parser_eat(parser, TOKEN_MINUS);
        parser_eat(parser, TOKEN_GT);

        ALLOC(function->return_type);
        *function->return_type = parser_parse_type(parser);
    }

    if (parser->token->type == TOKEN_LBRACE) {
        function->body = parser_parse_scope(parser);
    } else {
        parser_eat(parser, TOKEN_SEMI);
        function->body = NULL;
    }

    switch (ast->scope->type) {
        case AST_IMPL:
            ARENA_APPEND(&ast->scope->value.implementation.members, ast); break;
        case AST_TRAIT:
            ARENA_APPEND(&ast->scope->value.trait.children, ast); break;
        default: break;
    }

    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_import(struct Parser * parser) {
    parser_eat_keyword(parser, KEYWORD_IMPORT); // import


    String current_path = string_init(parser->path);
    int last = current_path.length - 1; // offset of last '/' in path
    while (--last, last > 0 && current_path._ptr[last] != '/');
    ASSERT1(last != -1);
    current_path._ptr[last + 1] = '\0';

    SourceSpan module_path = parser->token->span;
    const char * module_path_cstr = source_span_to_cstr(parser->token->span);

    char * formatted_path = format("{4s}", current_path._ptr, "/", module_path_cstr, ".fe");
    char * abs_path = get_abs_path(formatted_path);
    free_string(current_path);
    free(formatted_path);

    parser_eat(parser, TOKEN_STRING_LITERAL);

    parser_eat_keyword(parser, KEYWORD_AS);

    struct AST * this_module = get_scope(AST_MODULE, parser->current_scope);
    ASSERT(this_module != NULL, "Unable to find current module");
    ASSERT1(this_module == parser->current_scope);

    struct AST * imported_module = add_module(parser, abs_path);
    ASSERT1(imported_module != NULL);

    struct AST * import = init_ast(AST_IMPORT, parser->current_scope);
    import->value.import.name_id = parser->token->interner_id;
    import->value.import.module = imported_module;

    parser_eat(parser, TOKEN_ID);


    return import;
}

struct AST * parser_parse_identifier(struct Parser * parser) {
    ASSERT1(parser->token->type == TOKEN_ID);
    struct Keyword identifier = keyword_get_by_intern_id(parser->token->interner_id);

    if (identifier.flag != GLOBAL_ONLY && identifier.flag != ANY) {
        FATAL("[Parser] {s} is not allowed in the module scope", token_to_str(*parser->token));
    }

    switch (identifier.key) {
        case KEYWORD_NOT_FOUND:
            FATAL("[Parser]: {s} is not a valid identifier", token_to_str(*parser->token));
        case KEYWORD_FN:
            return parser_parse_function(parser);
        case KEYWORD_CONST:
        case KEYWORD_LET:
            return parser_parse_declaration(parser, identifier.key);
        case KEYWORD_IMPORT:
            return parser_parse_import(parser);
        case KEYWORD_STRUCT:
            return parser_parse_struct(parser);
        case KEYWORD_TRAIT:
            return parser_parse_trait(parser);
        case KEYWORD_IMPL:
            return parser_parse_impl(parser);
        default:
            FATAL("Unknown identifier: '{s}'", keyword_get_str(identifier.key));
    }
}

struct AST * parser_parse_module(struct Parser * parser, struct AST * ast) {
    struct AST * node = NULL;
    parser->current_scope = ast;
    a_module * module = &ast->value.module;

    while (parser->token->type != TOKEN_EOF) {
        if (parser->token->type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
            continue;
        }

        node = parser_parse_identifier(parser);
        parser->current_scope = ast;

        if (node == NULL) {
            continue;
        }

        ARENA_APPEND(&module->members, node);

        if (node->type == AST_DECLARATION) {
            a_declaration decl = node->value.declaration;
            a_expression expr = decl.expression->value.expression;
            struct AST * temp = ARENA_GET(expr.children, 0, struct AST *);
            if (temp->type != AST_VARIABLE) {
                FATAL("{2i::} Global variable declarations have to have variable as the LHS parameter", parser->token->line, parser->token->pos);
            }
        }

    }
    
    return ast;
}

void parser_parse(struct AST * root_ast, char * path) {
    struct Parser * parser = init_parser(path);
    parser->root = root_ast;
    parser->current_scope = root_ast;

    a_root * root = &root_ast->value.root;
    root->entry_point = path;

    add_module(parser, path);

    while (parser->modules_to_parse.size > 0) {
        path = ARENA_POP(&parser->modules_to_parse, char *);
        struct AST * module = find_module(root_ast, path);
        ASSERT1(module != NULL);

        INFO("Parsing module '{s}'", path);
        parser_change_path(parser, path);
        parser_parse_module(parser, module);
    }

}
