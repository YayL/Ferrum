#include "parser.h"

struct Parser * init_parser(struct Lexer * lexer) {
    
    struct Parser * parser = malloc(sizeof(struct Parser));

    parser->lexer = lexer;
    parser->token = lexer_next_token(lexer);

    return parser;
}

struct Keyword str_to_keyword(const char * str) {
    if (str == NULL) {
        println("ERROR: str_to_keyword str is NULL");
        exit(1);
    }

    for (int i = 0; i < sizeof(conversion) / sizeof(conversion[0]); ++i) {
        if (!strcmp(str, conversion[i].str))
            return conversion[i];
    }
    return conversion[0];
}

void parser_eat(struct Parser * parser, enum token_t type) {
    if (parser->token->type != type) {
        print_token("[Parser]: Unexpected token: {s}, ", parser->token);
        println("expecting: {s}\n", token_type_to_str(type));
        exit(1);
    }

    parser->prev = parser->token;
    parser->token = lexer_next_token(parser->lexer);
}

struct Ast * parser_parse_int(struct Parser * parser) {
    struct Ast * number = init_ast(AST_INT);
    number->name = parser->token->value;

    parser_eat(parser, TOKEN_INT);

    return number;
}

struct Ast * parser_parse_id(struct Parser * parser) {
    
    struct Ast * id = init_ast(AST_VARIABLE);
    id->name = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    if (parser->token->type == TOKEN_COLON) {
        parser_eat(parser, TOKEN_COLON);
        
        // TODO: Parse type here

        if (parser->token->type != TOKEN_ID) {
            print_token("[Parser] Unexepected variable type: {s}\n", parser->token);
            exit(1);
        }

        id->data_type = parser->token->value;
        parser_eat(parser, TOKEN_ID);
    }

    return id;
}

struct Ast * parser_parse_if(struct Parser * parser) {
    struct Ast * node = init_ast(AST_IF);
    struct Ast * temp;
    node->nodes = init_list(sizeof(struct Ast *));
    char is_else = 0;
    
    while (1) {
        temp = init_ast(AST_IF);
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);

        if (node->nodes->size && parser->token->type == TOKEN_ID && !strcmp(parser->token->value, "else")) {
            parser_eat(parser, TOKEN_ID);
            is_else = 1;
        }

        if (parser->token->type == TOKEN_ID && !strcmp(parser->token->value, "if")) {
            temp->left = parser_parse_expr(parser);
        } else {
            if (is_else) {
                temp->value = parser_parse_scope(parser);
                list_push(node->nodes, temp);
            } else {
                free_ast(temp);
            }
            break;
        }

        temp->value = parser_parse_scope(parser);
        list_push(node->nodes, temp);
        is_else = 0;
    }

    return node;
}

struct Ast * parser_parse_for(struct Parser * parser) {
    struct Ast * node = init_ast(AST_FOR);

    return node;
}

struct Ast * parser_parse_while(struct Parser * parser) {
    struct Ast * node = init_ast(AST_WHILE);
    struct Ast * expr = parser_parse_expr(parser);

    node->value = parser_parse_scope(parser);

    return node;
}

struct Ast * parser_parse_do(struct Parser * parser) {
    struct Ast * node = init_ast(AST_DO);

    return node;
}

struct Ast * parser_parse_match(struct Parser * parser) {
    struct Ast * node = init_ast(AST_MATCH);

    return node;
}

struct Ast * parser_parse_return(struct Parser * parser) {
    struct Ast * node = init_ast(AST_RETURN);
    parser_eat(parser, TOKEN_ID);
    
    node->value = parser_parse_expr(parser);

    return node;
}

struct Ast * parser_parse_declaration(struct Parser * parser) {
    struct Ast * node = init_ast(AST_DECLARE);
    parser_eat(parser, TOKEN_ID);

    node->value = parser_parse_expr(parser);

    return node;
}

struct Ast * parser_parse_statement(struct Parser * parser) {

    struct Ast * node;

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

    return NULL;
}

struct Ast * parser_parse_scope(struct Parser * parser) {
    struct Ast * scope = init_ast(AST_SCOPE);
    scope->nodes = init_list(sizeof(struct Ast *));
    
    parser_eat(parser, TOKEN_LBRACE);

    while (1) {
        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);
        
        if (parser->token->type == TOKEN_RBRACE)
            break;
        list_push(scope->nodes, parser_parse_statement(parser));
    }

    parser_eat(parser, TOKEN_RBRACE);

    return scope;
}

struct Ast * parser_parse_function(struct Parser * parser) {
    
    struct Ast * function = init_ast(AST_FUNCTION), * argument;
    function->variables = init_list(sizeof(struct Ast *));

    parser_eat(parser, TOKEN_ID);

    function->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);
    parser_eat(parser, TOKEN_LPAREN);

    // replace this with an expression call (perhaps?)

function_loop: 
    {
        argument = init_ast(AST_VARIABLE);
        argument->name = parser->token->value;
        argument->scope = function;
        
        parser_eat(parser, TOKEN_ID);
        parser_eat(parser, TOKEN_COLON);

        argument->str_value = parser->token->value;
        parser_eat(parser, TOKEN_ID);

        list_push(function->variables, argument);

        if (parser->token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            goto function_loop;
        }
    }

    parser_eat(parser, TOKEN_RPAREN);
    parser_eat(parser, TOKEN_OP);

    function->data_type = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    function->value = parser_parse_scope(parser);
    function->value->scope = function;

    return function;
}

struct Ast * parser_parse_package(struct Parser * parser) {

    return NULL;

}

struct Ast * parser_parse_identifier(struct Parser * parser) {

    struct Ast * node = NULL;
    
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
            break;
    }

    return node;
}

struct Ast * parser_parse_module(struct Parser * parser) {
    
    struct Ast * module = init_ast(AST_MODULE), * node;
    module->nodes = init_list(sizeof(struct Ast *));

    while (parser->token->type != TOKEN_EOF) {
        node = parser_parse_identifier(parser);
        node->scope = module;
        list_push(module->nodes, node);

        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);
    }
    
    return module;
}

struct Ast * parser_parse(struct Parser * parser) {
    return parser_parse_module(parser);
}
