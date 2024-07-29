#include "parser/parser.h"
#include "codegen/AST.h"
#include "common/hashmap.h"
#include "common/list.h"
#include "common/string.h"
#include "parser/modules.h"
#include <string.h>

struct Keyword str_to_keyword(const char * str) {
    if (str == NULL)
        return conversion[0];

    for (int i = 0; i < sizeof(conversion) / sizeof(conversion[0]); ++i) {
        if (!strcmp(str, conversion[i].str))
            return conversion[i];
    }

    return conversion[0];
}

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
        parser->error = 1;
    }

    if (parser->prev != NULL && parser->token != NULL)
        copy_token(parser->prev, parser->token);
    lexer_next_token(parser->lexer);
    parser->token = parser->lexer->tok;
}

struct Ast * parser_parse_int(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_LITERAL, parser->current_scope), * type_ast = init_ast(AST_TYPE, parser->current_scope);
    a_literal * number = ast->value;
    a_type * type = type_ast->value;

    Numeric_T * num = init_intrinsic_type(INumeric);
    num->type = NUMERIC_SIGNED;
    num->width = 32;

    type->name = "i32";
    type->ptr = num;
    type->intrinsic = INumeric;

    number->type = type_ast;
    number->value = parser->token->value;

    parser_eat(parser, TOKEN_INT);

    return ast;
}

struct Ast * parser_parse_string(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_LITERAL, parser->current_scope),
               * type_ast = init_ast(AST_TYPE, parser->current_scope);
    a_literal * string = ast->value;
    a_type * type = type_ast->value;

    Array_T * array = init_intrinsic_type(IArray);
    array->size = parser->token->length;
    
    a_type * basetype = init_ast_of_type(AST_TYPE);
    Numeric_T * num = init_intrinsic_type(INumeric);
    num->type = NUMERIC_UNSIGNED;
    num->width = 8;

    basetype->ptr = num;
    basetype->intrinsic = INumeric;

    array->basetype = basetype;

    type->ptr = array;
    type->intrinsic = IArray;

    string->type = type_ast;
    string->literal_type = LITERAL_STRING;
    string->value = parser->token->value;

    parser_eat(parser, TOKEN_STRING_LITERAL);

    return ast;
}

struct Ast * parser_parse_id(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_VARIABLE, parser->current_scope);
    a_variable * variable = ast->value;

    variable->name = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON); 
        variable->type = parser_parse_type(parser);
    }

    return ast;
}

struct Ast * parser_parse_if(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_IF, parser->current_scope);
    
    a_if_statement * if_statement = NULL,
                   * previous;
    char is_else = 0;
    
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        is_else = 0;

        if (if_statement && parser->token->type == TOKEN_ID && !strcmp(parser->token->value, "else")) {
            parser_eat(parser, TOKEN_ID);
            is_else = 1;
        }

        if (if_statement) {
            if_statement->next = init_ast_of_type(AST_IF);
            previous = if_statement;
            if_statement = if_statement->next;
        } else {
            if_statement = ast->value;
        }

        if (parser->token->type == TOKEN_ID && !strcmp(parser->token->value, "if")) {
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

struct Ast * parser_parse_for(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_FOR, parser->current_scope);
    a_for_statement * for_statement = ast->value;

    struct Token * for_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    
    for_statement->expression = parser_parse_expr(parser);
    for_statement->body = parser_parse_scope(parser);

    return ast;
}

struct Ast * parser_parse_while(struct Parser * parser) {
    struct Token * while_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    struct Ast * ast = init_ast(AST_WHILE, parser->current_scope);
    a_while_statement * while_statement = ast->value;
    
    while_statement->expression = parser_parse_expr(parser);
     
    while_statement->body = parser_parse_scope(parser);

    return ast;
}

struct Ast * parser_parse_do(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_DO, parser->current_scope);

    logger_log("Do statements are not implemented yet", PARSER, ERROR);
    exit(1);

    return ast;
}

struct Ast * parser_parse_match(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_MATCH, parser->current_scope);

    logger_log("Match statements are not implemented yet", PARSER, ERROR);
    exit(1);

    return ast;
}

struct Ast * parser_parse_return(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_RETURN, parser->current_scope);
    a_return * return_statement = ast->value;

    parser_eat(parser, TOKEN_ID);    
    return_statement->expression = parser_parse_expr(parser);

    return ast;
}

void parser_parse_template_types(struct Parser * parser, struct HashMap * map) { 
}

struct Ast * parser_parse_struct(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_STRUCT, parser->current_scope), * temp;
    a_struct * _struct = ast->value;
    
    parser_eat(parser, TOKEN_ID);
    _struct->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);

        while (1) {
            list_push(_struct->generics, init_string_with_length(parser->token->value, parser->token->length));
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
                        a_expr * expr = ((a_declaration *) temp->value)->expression->value;
                        for (int i = 0; i < expr->children->size; ++i) {
                            list_push(_struct->variables, list_at(expr->children, i));
                        }
                    } break;
                case AST_FUNCTION:
                    {
                        list_push(_struct->functions, temp);
                    } break;
                default:
                    {
                        logger_log(format("{2i::} Invalid identifier '{s}' in struct declaration", parser->lexer->line, parser->lexer->pos, ast_type_to_str(temp->type)), PARSER, ERROR);
                        exit(1);
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

struct Ast * parser_parse_trait(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_TRAIT, parser->current_scope),
               * node;
    
    parser->current_scope = ast;

    a_trait * trait = ast->value;
    parser_eat(parser, TOKEN_ID);
    trait->name = parser->token->value;
    add_marker(ast, trait->name);
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

struct Ast * parser_parse_impl(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_IMPL, parser->current_scope),
               * node,
               * type;
    a_impl * impl = ast->value;
    parser->current_scope = ast;

    impl->members = init_list(sizeof(struct Ast *));

    parser_eat(parser, TOKEN_ID);
    impl->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);

    parser_eat(parser, TOKEN_ID);

    impl->type = parser_parse_type(parser);

    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_RBRACE)
            break;

        node = parser_parse_identifier(parser);
        switch (node->type) {
            case AST_FUNCTION:
            {
                a_module * module = get_scope(AST_MODULE, parser->current_scope)->value;
                add_function_to_module(get_scope(AST_MODULE, parser->current_scope), node);
            }
            default:
                list_push(impl->members, node);
        }
    }

    parser_eat(parser, TOKEN_RBRACE);

    parser->current_scope = ast->scope;

    print_ast("{s}\n", ast);

    return ast;
}

struct Ast * parser_parse_declaration(struct Parser * parser, enum Keywords keyword) {
    struct Ast * ast = init_ast(AST_DECLARATION, parser->current_scope),
               * node;
    a_declaration * declaration = ast->value;
    declaration->is_const = keyword == CONST;

    parser_eat(parser, TOKEN_ID);


    declaration->expression = parser_parse_expr(parser);
    a_expr * expr = declaration->expression->value;

    node = list_at(expr->children, 0);
    if (node->type == AST_OP) {
        a_op * op = node->value;
        node = op->left;
    }

    if (node->type != AST_VARIABLE) {
        logger_log(format("{2i::} LHS of declaration must be a variable", parser->token->line, parser->token->pos), PARSER, ERROR);
        exit(1);
    }

    declaration->variable = node;

    return ast;
}

struct Ast * parser_parse_statement(struct Parser * parser) {
    struct Keyword keyword = str_to_keyword(parser->token->value);

    if (keyword.key == KEYWORD_NOT_FOUND) { // no identifier should mean there is an expression
        return parser_parse_expr(parser);
    }

    switch (keyword.key) {
        case CONST:
        case LET:
            return parser_parse_declaration(parser, keyword.key);
        case ELSE:
            print_token("[Parser] Error else without if: {s}\n", parser->token);
            exit(1);
        case IF:
            return parser_parse_if(parser);
        case FOR:
            return parser_parse_for(parser);
        case WHILE:
            return parser_parse_while(parser);
        case DO:
            return parser_parse_do(parser);
        case MATCH:
            return parser_parse_match(parser);
        case RETURN:
            return parser_parse_return(parser);
        default:
            if (keyword.flag == GLOBAL_ONLY && keyword.flag != ANY) {
                print_token("[Parser]: {s} is not allowed in the function scope\n", parser->prev);
                exit(1);
            }
    }
    println("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
    exit(1);
}

struct Ast * parser_parse_scope(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_SCOPE, parser->current_scope),
               * statement;
    a_scope * scope = ast->value;
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
                logger_log("Unclosed scope", PARSER, ERROR);
                exit(1);
            }

            list_push(scope->nodes, parser_parse_statement(parser));
        }

        parser_eat(parser, TOKEN_RBRACE);
    } else {
        if (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (parser->token->type == TOKEN_LINE_BREAK) {
            logger_log("Scopes without curly brackets must follow the scope initializer immidiatly(1 line or less)", PARSER, WARN);
        } else {
            list_push(scope->nodes, parser_parse_statement(parser));
        }
    }
    parser->current_scope = ast->scope;

    return ast;
}

struct Ast * parser_parse_function(struct Parser * parser) {
    
    struct Ast * ast = init_ast(AST_FUNCTION, parser->current_scope), 
               * argument,
               * param_types;
    struct a_function * function = ast->value;
    char * name;
    parser->current_scope = ast;
    
    parser_eat(parser, TOKEN_ID);
    
    if (parser->token->type && !strcmp("inline", parser->token->value)) {
        function->is_inline = 1;
        parser_eat(parser, TOKEN_ID);
    }

    if (parser->token->type == TOKEN_ID) {
        function->name = name = parser->token->value;
        parser_eat(parser, TOKEN_ID);
    }
    
    if (ast->scope->type == AST_IMPL || ast->scope->type == AST_TRAIT) {
        function->template_types = init_hashmap(3);
        function->parsed_templates = init_list(sizeof(char *));
        list_push(function->parsed_templates, "Self");
        struct Ast * type = NULL;
        if (ast->scope->type == AST_IMPL) {
            type = ((a_impl *) ast->scope->value)->type;
        }
        hashmap_set(function->template_types, "Self", type);
    }

    if (parser->token->type == TOKEN_LT) {
        parser_eat(parser, TOKEN_LT);
        
        if (function->template_types == NULL)  {
            function->template_types = init_hashmap(3);
            function->parsed_templates = init_list(sizeof(char *));
        }

        struct Ast * ast;
        char * name;

        while (parser->token->type == TOKEN_ID) {
            ast = parser_parse_id(parser);
            ASSERT1(ast->type == AST_VARIABLE);
            name = ((a_variable *) ast->value)->name;
            list_push(function->parsed_templates, name);
            hashmap_set(function->template_types, name, ast);

            if (parser->token->type == TOKEN_GT)
                break;
            parser_eat(parser, TOKEN_COMMA);
        }

        parser_eat(parser, TOKEN_GT);
    }

    parser_eat(parser, TOKEN_LPAREN);
    
    function->arguments = parser_parse_expr_exit_on(parser, PARENTHESES);
    ASSERT1(function->arguments->type == AST_EXPR);

    if (((a_expr *) function->arguments->value)->children->size != 0) {
        function->param_type = ast_to_type(function->arguments);
    }
    
    if (parser->token->type == TOKEN_MINUS) {
        parser_eat(parser, TOKEN_MINUS);
        parser_eat(parser, TOKEN_GT);

        function->return_type = parser_parse_type(parser);
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

struct Ast * parser_parse_package(struct Parser * parser) {
    
    parser_eat(parser, TOKEN_ID);

    char * path = parser->path;
    int last = 0;
    for (int i = 0; path[i] != 0; ++i) {
        if (path[i] == '/')
            last = i;
    }
    
    path[last] = 0;
    path = format("{4s}", path, "/", parser->token->value, ".fe");
    parser->path[last] = '/';

    char * name = parser->token->value;

    logger_log(format("Added package '{s}' at '{s}'", name, path), PARSER, INFO);
    parser_eat(parser, TOKEN_STRING_LITERAL);

    struct Ast * other_module = find_module(parser->root, path),
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

struct Ast * parser_parse_identifier(struct Parser * parser) {

    if (parser->token->value == NULL) {
        print_token("[Error] Unknown identifier\n{s}\n", parser->token);
        exit(1);
    }
    
    struct Keyword identifier = str_to_keyword(parser->token->value);

    if (identifier.flag != GLOBAL_ONLY && identifier.flag != ANY) {
        print_token("[Parser]: {s} is not allowed in the module scope\n", parser->token);
        exit(1);
    }
 
    switch (identifier.key) {
        case OP_NOT_FOUND:
            print_token("[Parser]: {s} is not a valid identifier\n", parser->token);
            exit(1);
        case FN:
            return parser_parse_function(parser);
        case CONST:
        case LET:
            return parser_parse_declaration(parser, identifier.key);
        case PACKAGE:
            return parser_parse_package(parser);
        case STRUCT:
            return parser_parse_struct(parser);
        case TRAIT:
            return parser_parse_trait(parser);
        case IMPL:
            return parser_parse_impl(parser);
        default:
            logger_log(format("Unknown identifier: '{s}'", identifier.str), PARSER, ERROR);
            exit(1);
    }
}

struct Ast * parser_parse_module(struct Parser * parser, struct Ast * ast) {
    struct Ast * node = NULL;
    parser->current_scope = ast;
    a_module * module = ast->value;

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
                a_module * module = get_scope(AST_MODULE, parser->current_scope)->value;
                add_function_to_module(get_scope(AST_MODULE, parser->current_scope), node);
                list_push(module->functions, node);
                hashmap_set(module->symbols, ((a_function *) node->value)->name, node);
                break;
            }
            case AST_DECLARATION:
            {
                list_push(module->variables, node);

                a_declaration * decl = node->value;
                a_expr * expr = decl->expression->value;
                struct Ast * temp = list_at(expr->children, 0);
                if (temp->type != AST_VARIABLE) {
                    logger_log(format("{2i::} Global variable declarations have to have variable as the LHS parameter", parser->token->line, parser->token->pos), PARSER, ERROR);
                    exit(1);
                }

                hashmap_set(module->symbols, ((a_variable *) temp->value)->name, node);
            } break;
            case AST_STRUCT:
                list_push(module->structures, node);
                hashmap_set(module->symbols, ((a_struct *) node->value)->name, node);
                break;
            case AST_TRAIT:
                list_push(module->traits, node);
                hashmap_set(module->symbols, ((a_trait *) node->value)->name, node);
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

struct Ast * parser_parse(struct Ast * root, char * path) {
    struct Parser * parser = init_parser(path);
    parser->root = root;
    parser->current_scope = root;

    struct Ast * module = init_ast(AST_MODULE, parser->current_scope);
    ((a_module *)module->value)->path = path;

    list_push(((a_root *) root->value)->modules, module);
    parser_parse_module(parser, module);

    return root;
}
