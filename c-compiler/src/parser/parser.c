#include "parser/parser.h"

#include "codegen/AST.h"
#include "common/common.h"
#include "common/io.h"
#include "parser/modules.h"
#include "parser/lexer.h"

struct Parser * init_parser(char * path) {
    struct Parser * parser = malloc(sizeof(struct Parser));

    size_t length;
    char * src = read_file(path, &length);
    struct Lexer * lexer = init_lexer(src, length);

    parser->path = path;
    parser->lexer = lexer;
    parser->error = 0;
    parser->prev = init_token();

    lexer_next_token(lexer);
    parser->token = lexer->tok;
   
    return parser;
}

void parser_eat(struct Parser * parser, enum token_t type) {
    if (parser->token->type != type) {
        println("[Error] {2i::} Expected token type '{s}' got token '{s}'", 
                    parser->token->line, parser->token->pos, 
                    token_type_to_str(type), 
                    token_type_to_str(parser->token->type));
        println("path: {s}", parser->path);
        parser->error = 1;
    }

    if (parser->prev != NULL && parser->token != NULL)
        copy_token(parser->prev, parser->token);
    lexer_next_token(parser->lexer);
    parser->token = parser->lexer->tok;
}

struct AST * parser_parse_int(struct Parser * parser) {
    struct AST * ast = init_ast(AST_LITERAL, parser->current_scope);
    a_literal * number = &ast->value.literal;

    Numeric_T num = init_intrinsic_type(INumeric).numeric;
    num.type = NUMERIC_SIGNED;
    num.width = 32;

    ALLOC(number->type);
    Type * type = number->type;

    type->name_id = interner_intern(STRING_FROM_LITERAL("i32"));
    type->value.numeric = num;
    type->intrinsic = INumeric;

    number->value = parser->token->value.span;

    parser_eat(parser, TOKEN_INT);

    return ast;
}

struct AST * parser_parse_string(struct Parser * parser) {
    struct AST * ast = init_ast(AST_LITERAL, parser->current_scope);
    a_literal * string = &ast->value.literal;

    ALLOC(string->type);
    Type * type = string->type;

    Array_T array = init_intrinsic_type(IArray).array;
    array.size = parser->token->value.span.length;
    array.basetype = malloc(sizeof(Type));
    
    Numeric_T num = init_intrinsic_type(INumeric).numeric;
    num.type = NUMERIC_UNSIGNED;
    num.width = 8;

    array.basetype->value.numeric = num;
    array.basetype->intrinsic = INumeric;

    type->value.array = array;
    type->intrinsic = IArray;

    string->literal_type = LITERAL_STRING;
    string->value = parser->token->value.span;

    parser_eat(parser, TOKEN_STRING_LITERAL);

    return ast;
}

struct AST * parser_parse_id(struct Parser * parser) {
    struct AST * ast = init_ast(AST_VARIABLE, parser->current_scope);
    a_variable * variable = &ast->value.variable;

    variable->interner_id = parser->token->value.interner_id;

    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON); 
        ALLOC(variable->type);
        *variable->type = parser_parse_type(parser);
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

        if (if_statement && parser->token->type == TOKEN_ID && parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_ELSE)) {
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

        if (parser->token->type == TOKEN_ID && parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_IF)) {
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
    a_return * return_statement = &ast->value.return_statement;

    parser_eat(parser, TOKEN_ID);    
    return_statement->expression = parser_parse_expr(parser);

    return ast;
}

void parser_parse_template_types(struct Parser * parser, struct hashmap * map) { 
}

struct AST * parser_parse_struct(struct Parser * parser) {
    struct AST * ast = init_ast(AST_STRUCT, parser->current_scope), * temp;
    a_struct * _struct = &ast->value.structure;
    
    parser_eat(parser, TOKEN_ID);
    _struct->interner_id = parser->token->value.interner_id;
    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);

        while (1) {
            // list_push(_struct->generics, init_string_with_length(parser->token->value, parser->token->length));
            parser_eat(parser, TOKEN_ID);
            if (parser->token->type == TOKEN_GT)
                break;
            parser_eat(parser, TOKEN_COMMA);
        }

        parser_eat(parser, TOKEN_GT);
    }

    parser_eat(parser, TOKEN_LBRACE);
    parser_eat(parser, TOKEN_LINE_BREAK);

    do {
        temp = parser_parse_identifier(parser);
        if (temp != NULL) {
            switch (temp->type) {
                case AST_DECLARATION:
                    {
                        a_expr expr = temp->value.declaration.expression->value.expression;
                        for (int i = 0; i < expr.children->size; ++i) {
                            list_push(_struct->variables, list_at(expr.children, i));
                        }
                    } break;
                case AST_FUNCTION:
                    {
                        list_push(_struct->functions, temp);
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
    parser_eat(parser, TOKEN_ID);
    trait->interner_id = parser->token->value.interner_id;
    // add_marker(ast, trait->name);
    parser_eat(parser, TOKEN_ID);

    parser_eat(parser, TOKEN_LBRACE);
    
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_RBRACE)
            break;
        
        node = parser_parse_identifier(parser);
        if (node != NULL)
            list_push(trait->children, node);
    }

    parser_eat(parser, TOKEN_RBRACE);
    
    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_impl(struct Parser * parser) {
    struct AST * ast = init_ast(AST_IMPL, parser->current_scope),
               * node,
               * type;
    a_impl * impl = &ast->value.implementation;
    parser->current_scope = ast;

    parser_eat(parser, TOKEN_ID); // impl
    impl->interner_id = parser->token->value.interner_id;
    parser_eat(parser, TOKEN_ID); // [name]

    parser_eat(parser, TOKEN_ID); // for

    ALLOC(impl->type);
    *impl->type = parser_parse_type(parser);

    // DEBUG("\"{s}\" {s", impl->name, type_to_str(*impl->type));

    parser_eat(parser, TOKEN_LBRACE);

    impl->members = init_list(sizeof(struct AST *));
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_RBRACE)
            break;

        node = parser_parse_identifier(parser);
        switch (node->type) {
            case AST_FUNCTION:
            {
                // a_module * module = get_scope(AST_MODULE, parser->current_scope)->value;
                add_function_to_module(get_scope(AST_MODULE, parser->current_scope), node);
                DEBUG("Added function: {s}", ast_to_string(node));
            }
            default:
                list_push(impl->members, node);
        }
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
    a_expr expr = declaration->expression->value.expression;

    node = list_at(expr.children, 0);
    if (node->type == AST_OP) {
        a_op op = node->value.operator;
        node = op.left;
    }

    if (node->type != AST_VARIABLE) {
        FATAL("{2i::} LHS of declaration must be a variable", parser->token->line, parser->token->pos);
    }

    declaration->variable = node;

    return ast;
}

struct AST * parser_parse_statement(struct Parser * parser) {
    if (parser->token->type != TOKEN_ID) {
        return parser_parse_expr(parser);
    }

    struct Keyword keyword = keyword_get(parser->token->value.interner_id);
    ASSERT(keyword.key == KEYWORD_NOT_FOUND, "Unable to find keyword '{u}'?", parser->token->value.interner_id);

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
                FATAL("[Parser] {s} is not allowed in the function scope\n", token_to_str(*parser->prev));
            }
    }
    println("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
    exit(1);
}

struct AST * parser_parse_scope(struct Parser * parser) {
    struct AST * ast = init_ast(AST_SCOPE, parser->current_scope), * statement;
    a_scope scope = ast->value.scope;
    parser->current_scope = ast;
    
    if (parser->token->type == TOKEN_LBRACE) {
        parser_eat(parser, TOKEN_LBRACE);

        while (1) {
            while (parser->token->type == TOKEN_LINE_BREAK) {
                parser_eat(parser, TOKEN_LINE_BREAK);
            }

            if (parser->token->type == TOKEN_RBRACE)
                break;
            else if (parser->token->type == TOKEN_EOF) {
                FATAL("Unclosed scope");
            }

            list_push(scope.nodes, parser_parse_statement(parser));
        }

        parser_eat(parser, TOKEN_RBRACE);
    } else {
        if (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_LINE_BREAK) {
            WARN("Scopes without curly brackets must follow the scope initializer immidiatly(1 line or less)");
        } else {
            list_push(scope.nodes, parser_parse_statement(parser));
        }
    }
    parser->current_scope = ast->scope;

    return ast;
}

struct AST * parser_parse_function(struct Parser * parser) {
    struct AST * ast = init_ast(AST_FUNCTION, parser->current_scope), 
               * argument,
               * param_types;
    a_function * function = &ast->value.function;
    parser->current_scope = ast;

    if (parser->token->type == TOKEN_ID && parser->token->value.interner_id == keyword_get_intern_id(KEYWORD_INLINE)) {
        function->is_inline = 1;
        parser_eat(parser, TOKEN_ID);
    }

    if (parser->token->type == TOKEN_ID) {
        function->interner_id = parser->token->value.interner_id;
        parser_eat(parser, TOKEN_ID);
    }
    
    if (ast->scope->type == AST_IMPL || ast->scope->type == AST_TRAIT) {
        // function->template_types = hashmap_init(3);
        // function->parsed_templates = init_list(sizeof(char *));
        // list_push(function->parsed_templates, "Self");
        Type * type = NULL;
        if (ast->scope->type == AST_IMPL) {
            type = ast->scope->value.implementation.type;
        }

        // hashmap_set(function->template_types, "Self", type);
    }

    if (parser->token->type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);
        
        if (function->template_types == NULL)  {
            // function->template_types = hashmap_init(3);
            // function->parsed_templates = init_list(sizeof(char *));
        }

        struct AST * ast;
        char * name;

        while (parser->token->type == TOKEN_ID) {
            ast = parser_parse_id(parser);
            ASSERT1(ast->type == AST_VARIABLE);
            // list_push(function->parsed_templates, name);
            // hashmap_set(function->template_types, name, ast);

            if (parser->token->type == TOKEN_GT)
                break;
            parser_eat(parser, TOKEN_COMMA);
        }

        parser_eat(parser, TOKEN_GT);
    }

    parser_eat(parser, TOKEN_LPAREN);
    
    function->arguments = parser_parse_expr_exit_on(parser, PARENTHESES);
    ASSERT1(function->arguments->type == AST_EXPR);

    if (function->arguments->value.expression.children->size != 0) {
        ALLOC(function->param_type);
        *function->param_type = ast_to_type(function->arguments);
    }
    
    if (parser->token->type == TOKEN_MINUS) {
        parser_eat(parser, TOKEN_MINUS);
        parser_eat(parser, TOKEN_GT);

        ALLOC(function->return_type);
        *function->return_type = parser_parse_type(parser);
    }

    if (parser->token->type == TOKEN_SEMI) {
        parser_eat(parser, TOKEN_SEMI);
        function->body = NULL;
    } else {
        function->body = parser_parse_scope(parser);
    }
    parser->current_scope = ast->scope;

    // Add the function to the modules list of function
    return ast;
}

struct AST * parser_parse_package(struct Parser * parser) {
    parser_eat(parser, TOKEN_ID);

    char * path = parser->path;
    int last = 0;
    for (int i = 0; path[i] != 0; ++i) {
        if (path[i] == '/')
            last = i;
    }
    
    SourceSpan package_name = parser->token->value.span;
    const char * package_name_cstr = source_span_to_cstr(parser->token->value.span);

    path[last] = 0;
    path = format("{4s}", path, "/", package_name_cstr, ".fe");
    parser->path[last] = '/';


    INFO("Added package '{s}' at '{s}'", package_name_cstr, path);
    parser_eat(parser, TOKEN_STRING_LITERAL);

    struct AST * other_module = find_module(parser->root, path),
               * this_module = get_scope(AST_MODULE, parser->current_scope);
    
    if (other_module != NULL) {
        include_module(this_module, other_module);
    } else {
        parser_parse(parser->root, path);
        other_module = find_module(parser->root, path);
        include_module(this_module, find_module(parser->root, path));
    }
    
    return NULL;
}

struct AST * parser_parse_identifier(struct Parser * parser) {
    ASSERT1(parser->token->type == TOKEN_ID);
    struct Keyword identifier = keyword_get_by_intern_id(parser->token->value.interner_id);


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
        case KEYWORD_PACKAGE:
            return parser_parse_package(parser);
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

        if (node == NULL)
            continue;

        switch (node->type) {
            case AST_FUNCTION:
            {
                a_module * module = &get_scope(AST_MODULE, parser->current_scope)->value.module;
                add_function_to_module(get_scope(AST_MODULE, parser->current_scope), node);
                list_push(module->functions, node);
                // hashmap_set(module->symbols, node->value.function.name, node);
                break;
            }
            case AST_DECLARATION:
            {
                list_push(module->variables, node);

                a_declaration decl = node->value.declaration;
                a_expr expr = decl.expression->value.expression;
                struct AST * temp = list_at(expr.children, 0);
                if (temp->type != AST_VARIABLE) {
                    FATAL("{2i::} Global variable declarations have to have variable as the LHS parameter", parser->token->line, parser->token->pos);
                }

                // hashmap_set(module->symbols, temp->value.variable.name, node);
            } break;
            case AST_STRUCT:
                list_push(module->structures, node);
                // hashmap_set(module->symbols, node->value.structure.name, node);
                break;
            case AST_TRAIT:
                list_push(module->traits, node);
                // hashmap_set(module->symbols, node->value.trait.name, node);
                break;
            case AST_IMPL:
                list_push(module->impls, node);
                break;
            default:
                println("probably a package but exiting!");
                exit(1);
        }

    }
    
    return ast;
}

struct AST * parser_parse(struct AST * root, char * path) {
    struct Parser * parser = init_parser(path);
    parser->root = root;
    parser->current_scope = root;

    struct AST * module = init_ast(AST_MODULE, parser->current_scope);
    module->value.module.path = path;

    list_push(root->value.root.modules, module);
    parser_parse_module(parser, module);

    return root;
}
