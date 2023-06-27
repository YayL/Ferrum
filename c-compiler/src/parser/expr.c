#include "parser.h"

/*
 * Example:
 *
 * if 1 == 1 {
 * 
 *
 */

struct Operator * get_operator(const char * str, struct Token * token, enum OP_mode mode) {
    // Can differentiate Unary and Binary operators 
    // by checking the parser->prev value to 
    // see whether an ID or integer preceeded 
    // it to get a binary operator

    struct Operator * op = malloc(sizeof(struct Operator));
    *op = str_to_operator(str, mode);

    if (op->key == OP_NOT_FOUND) {
        print("[Parser]: Operator '{s}' not found around: ", str);
        print_token("{s}\n", token);
        exit(1);
    }

    return op;
}

void add_operator_from_expr(struct Operator * op, struct List * list) {
    struct Ast * node = init_ast(AST_OP);
    node->operator = op;
    node->name = op->str;
    list_push(list, node);
}

// function that actually does the parsing of the expression
//
// let a: i32 = 123 - 2 * pow(1, 2, 3);

struct List * _parser_parse_expr(struct Parser * parser, struct List * output, struct Deque * operators, char end_on_last_rparen) {
    struct Ast * node, * temp;
    struct List * expressions = init_list(sizeof(struct Ast *));

    size_t offset = 0;
    
    enum OP_mode mode = UNARY;
    struct Operator * op1, * op2;

    while (1) {
        switch (parser->token->type) {
            case TOKEN_ID:
            {
                node = parser_parse_id(parser);
                if (parser->token->type == TOKEN_LPAREN) {
                    // free deque
                    struct Deque * temp_d = init_deque(sizeof(struct Operator *));
                    struct List * temp_l = init_list(sizeof(struct Ast *));
                    node->nodes = _parser_parse_expr(parser, temp_l, temp_d, 1);

                    node->type = AST_CALL;
                }

                list_push(output, node);
                mode = BINARY;
                break;
            }
            case TOKEN_INT:
            {
                list_push(output, parser_parse_int(parser));

                mode = BINARY;
                break;
            }
            case TOKEN_OP:
            {
                op1 = get_operator(parser->token->value, parser->token, mode);
                op2 = deque_back(operators);

                while (op2 && (op2->key != LPAREN && 
                                (op2->precedence < op1->precedence 
                                 || (op1->precedence == op2->precedence && op2->associativity == LEFT))))
                {
                    add_operator_from_expr(op2, output);
                    pop_back(operators);
                    op2 = deque_back(operators);
                }
                push_back(operators, op1);

                mode = UNARY;
                parser_eat(parser, TOKEN_OP);
                break;
            }
            case TOKEN_SEMI:
            case TOKEN_COMMA:
            {
                while (operators->size && (op1 = deque_back(operators))->key != LPAREN) {
                    add_operator_from_expr(op1, output);
                    pop_back(operators);
                }

                if (operators->size > 1) {
                    print_token("[Parser] Invalid level of token: {s}\n", parser->token);
                    exit(1);
                }

                temp = init_ast(AST_OP);
                temp->nodes = output;
                list_push(expressions, temp);
                output = init_list(sizeof(struct Ast *));

                mode = UNARY;
                parser_eat(parser, parser->token->type);
                break;
            }
            case TOKEN_LPAREN:
            {
                op1 = get_operator("(", parser->token, UNARY);
                push_back(operators, op1);

                mode = UNARY;
                parser_eat(parser, TOKEN_LPAREN);
                break;
            }
            case TOKEN_RPAREN:
            {
                while ((op1 = deque_back(operators))->key != LPAREN) {
                    if (operators->size == 1) {
                        print_token("[Parser] Unmatched right parenthesis: \n{s}\n", parser->token);
                        exit(1);
                    }
                    add_operator_from_expr(op1, output);
                    pop_back(operators);
                }
                pop_back(operators);
                parser_eat(parser, TOKEN_RPAREN);

                if (end_on_last_rparen && operators->size == 0)
                    goto exit;
                
                mode = BINARY;
                break;
            }
            case TOKEN_EOF:
            case TOKEN_LINE_BREAK:
            case TOKEN_LBRACE:
                goto exit;
            default:
                print_token("[Parser]: Uncrecognized token in expression\n{s}\n", parser->token);
                exit(1);
        }
    }

exit:

    while (operators->size) {
        op1 = deque_back(operators);
        if (op1->key == LPAREN) {
            print_token("[Parser] Unmatched left parenthesis near:\n{s}\n", parser->token);
            exit(1);
        }
        add_operator_from_expr(op1, output);
        pop_back(operators);
    }

    temp = init_ast(AST_OP);
    temp->nodes = output;
    list_push(expressions, temp);

    return expressions;

}

struct Ast * parser_parse_expr(struct Parser * parser) {
    struct Ast * node = init_ast(AST_EXPR);

    struct List * output = init_list(sizeof(struct Ast *));
    struct Deque * operators = init_deque(sizeof(struct Operator *));

    node->nodes = _parser_parse_expr(parser, output, operators, 0);

    /* struct Ast * e; */
    /*  */
    /* for (int i = 0; i < output->size; ++i) { */
    /*     e = list_at(output, i); */
    /*     print_ast("{s}\n", e); */
    /*     if (e->type == AST_OP) { */
    /*         print("{s} ", e->operator->str); */
    /*     } else { */
    /*         print("{s} ", e->name); */
    /*     } */
    /* } */
    /*  */
    /* println(""); */

    return node;
}
