#include "parser/parser.h"

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
    parser->prev = NULL;
    parser->token = lexer_next_token(lexer);

    return parser;
}

void parser_eat(struct Parser * parser, enum token_t type) {
    if (parser->token->type != type) {
        println("[Error] {2i::} Expected token type '{s}' got token '{s}'", 
                    parser->token->pos, parser->token->line, 
                    token_type_to_str(type), 
                    token_type_to_str(parser->token->type));
        parser->error = 1;
    }

    parser->prev = parser->token;
    parser->token = lexer_next_token(parser->lexer);
}

struct Ast * parser_parse_int(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_LITERAL);
    a_literal * number = ast->value;

    number->type = NUMBER;
    number->value = parser->token->value;

    parser_eat(parser, TOKEN_INT);

    return ast;
}

struct Ast * parser_parse_id(struct Parser * parser) {
    
    struct Ast * ast = init_ast(AST_VARIABLE);
    a_variable * variable = ast->value;

    variable->name = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_ID) {

    }

    if (parser->token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        
        // TODO: Parse type here

        if (parser->token->type != TOKEN_ID) {
            print_token("[Parser] Unexepected variable type: {s}\n", parser->token);
            exit(1);
        }

        variable->type = parser->token->value;
        parser_eat(parser, TOKEN_ID);
    }

    return ast;
}

struct Ast * parser_parse_if(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_IF);
    
    a_if_statement * if_statement = NULL,
                   * previous;
    char is_else = 0;
    
    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

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
        } else {
            if (is_else) {
                if_statement->expression = parser_parse_scope(parser);
                if_statement->body = parser_parse_scope(parser);
            } else {
                previous->next = NULL;
                free(if_statement);
            }
            break;
        }

        if_statement->body = parser_parse_scope(parser);
        is_else = 0;
    }

    return ast;
}

struct Ast * parser_parse_for(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_FOR);
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
    struct Ast * ast = init_ast(AST_WHILE);
    a_while_statement * while_statement = ast->value;
    
    while_statement->expression = parser_parse_expr(parser);

    // while without an expression
    if (while_statement->expression->type != AST_CALL) {
        print_token("[Parser] Invalid while statement: {s}\n", while_token);
        exit(1);
    }
    
    while_statement->body = parser_parse_scope(parser);

    return ast;
}

struct Ast * parser_parse_do(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_DO);

    return ast;
}

struct Ast * parser_parse_match(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_MATCH);

    return ast;
}

struct Ast * parser_parse_return(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_RETURN);
    a_return * return_statement = ast->value;

    parser_eat(parser, TOKEN_ID);    
    return_statement->expression = parser_parse_expr(parser);

    return ast;
}

struct Ast * parser_parse_declaration(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_DECLARATION);
    a_declaration * declaration = ast->value;

    parser_eat(parser, TOKEN_ID);

    declaration->expression = parser_parse_expr(parser);

    return ast;
}

struct Ast * parser_parse_statement(struct Parser * parser) {
    struct Keyword keyword = str_to_keyword(parser->token->value);

    if (keyword.key == KEYWORD_NOT_FOUND) { // variable assignment probably
        return parser_parse_expr(parser);
    }

    switch (keyword.key) {
        case CONST:
        case LET:
            return parser_parse_declaration(parser);
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
                println("[Parser]: {s} is not allowed in the function scope", parser->prev);
                exit(1);
            }
    }
    println("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
    exit(1);
}

struct Ast * parser_parse_scope(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_SCOPE),
               * statement;
    a_scope * scope = ast->value;
    
    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);
        
        if (parser->token->type == TOKEN_RBRACE)
            break;

        statement = parser_parse_statement(parser);
        statement->scope = ast;
        list_push(scope->nodes, statement);
    }

    parser_eat(parser, TOKEN_RBRACE);

    return ast;
}

struct Ast * parser_parse_function(struct Parser * parser) {
    
    struct Ast * ast = init_ast(AST_FUNCTION), 
               * argument;
    struct a_function * function = ast->value;

    parser_eat(parser, TOKEN_ID);

    function->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);
    parser_eat(parser, TOKEN_OP);

    // replace this with an expression call (perhaps?)

function_loop: 
    {
        argument = init_ast(AST_VARIABLE);
        argument->scope = ast;
        ((a_variable *) argument->value)->name = parser->token->value;
        
        parser_eat(parser, TOKEN_ID);
        parser_eat(parser, TOKEN_COLON);

        ((a_variable *) argument->value)->type = parser->token->value;
        parser_eat(parser, TOKEN_ID);

        list_push(function->arguments, argument);

        if (parser->token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            goto function_loop;
        }
    }
    
    parser_eat(parser, TOKEN_OP);
    parser_eat(parser, TOKEN_OP);

    function->type = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    function->body = parser_parse_scope(parser);
    function->body->scope = ast;

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

    println("[Info] Added package '{s}' at '{s}'", parser->token->value, path);
    parser_eat(parser, TOKEN_STRING_LITERAL);

    a_module * temp;
    a_root * root = parser->root->value;

    for (int i = 0; i < root->modules->size; ++i) {
        temp = ((struct Ast *) root->modules->items[i])->value;

        if (!strcmp(path, temp->path)) {
            free(path);
            return NULL;
        }
    }

    parser_parse(parser->root, path);

    return NULL;
}

struct Ast * parser_parse_identifier(struct Parser * parser) {

    if (parser->token->value == NULL) {
        print_token("[Error] Unknown identifier\n{s}\n", parser->token);
        exit(1);
    }
    
    struct Keyword identifier = str_to_keyword(parser->token->value);

    if (identifier.flag != GLOBAL_ONLY && identifier.flag != ANY) {
        println("[Parser]: {s} is not allowed in the module scope", parser->token);
        exit(1);
    }

    switch (identifier.key) {
        case OP_NOT_FOUND:
            print_token("[Parser]: {s} is not a valid identifier", parser->token);
            exit(1);
        case FN:
            return parser_parse_function(parser);
        case CONST: // TODO: Differentiate between const and let
        case LET:
            return parser_parse_declaration(parser);
        case PACKAGE:
            return parser_parse_package(parser);
        default:
            logger_log("Unknown identifier", PARSER, ERROR);
            exit(1);
    }
}

struct Ast * parser_parse_module(struct Parser * parser, struct Ast * ast) {
    struct Ast * node;
    a_module * module = ast->value;

    while (parser->token->type != TOKEN_EOF) {
        if (parser->token->type == TOKEN_LINE_BREAK) {
            parser_eat(parser, TOKEN_LINE_BREAK);
            continue;
        }

        node = parser_parse_identifier(parser);

        if (node == NULL)
            continue;
        
        switch (node->type) {
            case AST_FUNCTION:
                node->scope = ast;
                list_push(module->functions, node);
                break;
            case AST_DECLARATION:
                node->scope = ast;
                list_push(module->variables, node);
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

    struct Ast * module = init_ast(AST_MODULE);
    ((a_module *)module->value)->path = path;

    list_push(((a_root *) root->value)->modules, module);
    parser_parse_module(parser, module);

    return root;
}
