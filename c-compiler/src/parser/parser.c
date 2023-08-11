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
    struct Ast * number = init_ast(AST_INT);
    number->name = parser->token->value;

    parser_eat(parser, TOKEN_INT);

    return number;
}

struct Ast * parser_parse_id(struct Parser * parser) {
    
    struct Ast * id = init_ast(AST_VARIABLE);
    id->name = parser->token->value;

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
            parser_eat(parser, TOKEN_ID);
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
    struct Token * for_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    struct Ast * node = init_ast(AST_FOR);
    
    node->left = parser_parse_expr(parser);
    node->value = parser_parse_scope(parser);

    return node;
}

struct Ast * parser_parse_while(struct Parser * parser) {
    struct Token * while_token = parser->token;
    parser_eat(parser, TOKEN_ID);
    struct Ast * node = parser_parse_statement_expr(parser);

    // while without an expression
    if (node->type != AST_CALL) {
        print_token("[Parser] Invalid while statement: {s}\n", while_token);
        exit(1);
    }
    
    node->value = parser_parse_scope(parser);
    node->type = AST_WHILE;

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
    println("[Parser] Unknown error control flow somehow got to the end of parser_parse_statement?");
    exit(1);
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
    function->nodes = init_list(sizeof(struct Ast *));

    parser_eat(parser, TOKEN_ID);

    function->name = parser->token->value;
    parser_eat(parser, TOKEN_ID);
    parser_eat(parser, TOKEN_OP);

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

        list_push(function->nodes, argument);

        if (parser->token->type == TOKEN_COMMA) {
            parser_eat(parser, TOKEN_COMMA);
            goto function_loop;
        }
    }
    
    parser_eat(parser, TOKEN_OP);
    parser_eat(parser, TOKEN_OP);

    function->data_type = parser->token->value;

    parser_eat(parser, TOKEN_ID);

    function->value = parser_parse_scope(parser);
    function->value->scope = function;

    return function;
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

    println("[Info] Added package {s} at {s}", parser->token->value, path);
    parser_eat(parser, TOKEN_STRING_LITERAL);

    struct Ast * temp;

    for (int i = 0; i < parser->root->nodes->size; ++i) {
        temp = parser->root->nodes->items[i];
        if (!strcmp(path, temp->name)) {
            free(path);
            return NULL;
        }
    }

    parser_parse(parser->root, path);

    return NULL;
}

struct Ast * parser_parse_identifier(struct Parser * parser) {

    struct Ast * node = NULL;

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
            break;
    }

    return node;
}

struct Ast * parser_parse_module(struct Parser * parser, struct Ast * module) {

    struct Ast * node;
    module->nodes = init_list(sizeof(struct Ast *));

    while (parser->token->type != TOKEN_EOF) {
        node = parser_parse_identifier(parser);

        if (node != NULL) {
            node->scope = module;
            list_push(module->nodes, node);
        }

        while (parser->token->type == TOKEN_LINE_BREAK)
            parser_eat(parser, TOKEN_LINE_BREAK);
    }
    
    return module;
}

struct Ast * parser_parse(struct Ast * root, char * path) {
    struct Parser * parser = init_parser(path);
    parser->root = root;

    struct Ast * module = init_ast(AST_MODULE);
    module->name = path;

    list_push(root->nodes, module);
    parser_parse_module(parser, module);

    return root;
}
