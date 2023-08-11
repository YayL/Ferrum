#include "parser/parser.h"

// TODO:
//
// Make all operators have a left and right side (well unless they are unary of course)
// so that it is easier to recognize LHS and RHS for assignments[??]

struct Ast * parser_parse_statement_expr(struct Parser * parser) {
    struct Ast * expr = parser_parse_expr(parser);

    struct Ast * statement = list_at(expr->nodes, 0);
    statement = list_at(statement->nodes, 0);

    free_ast(list_at(expr->nodes, 0));
    free_ast(expr);

    statement->left = init_ast(AST_EXPR);
    statement->left->nodes = statement->nodes;
    statement->nodes = NULL;

    return statement;
}

struct Operator * get_operator(const char * str, struct Token * token, enum OP_mode mode, char * enclosed_flag) {
    struct Operator * op = malloc(sizeof(struct Operator));
    *op = str_to_operator(str, mode, enclosed_flag);

    if (op->key == OP_NOT_FOUND) {
        print("[Parser]: {s} operator '{s}' not found: ", mode == BINARY ? "Binary" : "Unary", str);
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

struct List * _parser_parse_expr(struct Parser * parser, struct List * output, struct Deque * operators) {
    struct Ast * node, * temp;
    struct List * expressions = init_list(sizeof(struct Ast *));
    
    enum OP_mode mode = UNARY;
    struct Operator * op1, * op2;

    char enclosed_flag;

    while (1) {
        switch (parser->token->type) {
            case TOKEN_ID:
            {
                if (mode == BINARY) {
                    print_token("[Parser] Invalid expression token: {s}\n", parser->token);
                    exit(1);
                }
                node = parser_parse_id(parser);
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
                op1 = get_operator(parser->token->value, parser->token, mode, &enclosed_flag);

                if (op1->enclosed == ENCLOSED) {
                    if (!enclosed_flag) {
                        parser_eat(parser, parser->token->type);
                        
                        struct Deque * temp_d = init_deque(sizeof(struct Operator *));
                        push_back(temp_d, op1);
                        struct List * temp_l = init_list(sizeof(struct Ast *));

                        node = init_ast(op1->ast_type);
                        temp = init_ast(AST_EXPR);
                        node->operator = op1;

                        if (op1->mode == BINARY) {
                            node->left = list_at(output, -1);
                            list_pop(output);
                        }
                        
                        temp->nodes = _parser_parse_expr(parser, temp_l, temp_d);
                        node->value = temp;
                        
                        list_push(output, node);
                        
                        mode = BINARY;
                        break;
                    } else {
                        while (strcmp(op1->str, (op2 = deque_back(operators))->str)) {
                            if (operators->size == 1) {
                                print_token("[Parser] Unmatched enclosed operator: {s}\n", parser->token);
                                exit(1);
                            }
                            add_operator_from_expr(op2, output);
                            pop_back(operators);
                        }
                       
                        pop_back(operators);
                        parser_eat(parser, parser->token->type);
                        goto exit;
                    }
                }

                op2 = deque_back(operators);

                while (op2 && (op2->enclosed != ENCLOSED && 
                                (op2->precedence < op1->precedence 
                                 || (op1->precedence == op2->precedence && op2->associativity == LEFT))))
                {
                    add_operator_from_expr(op2, output);
                    pop_back(operators);
                    op2 = deque_back(operators);
                }

                push_back(operators, op1);

                mode = UNARY;
                parser_eat(parser, parser->token->type);
                break;
            }
            case TOKEN_SEMI:
            case TOKEN_COMMA:
            {
                while (operators->size && (op2 = deque_back(operators))->enclosed != ENCLOSED) {
                    add_operator_from_expr(op2, output);
                    pop_back(operators);
                }

                temp = init_ast(AST_EXPR);
                temp->nodes = output;
                list_push(expressions, temp);
                output = init_list(sizeof(struct Ast *));

                mode = UNARY;
                parser_eat(parser, parser->token->type);
                break;
            }
            case TOKEN_LINE_BREAK:
                if (parser->prev->type == TOKEN_BACKSLASH)
                    break;
            case TOKEN_EOF:
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
        if (op1->enclosed == ENCLOSED) {
            print_token("[Parser] Unmatched enclosed operator near: {s}\n", parser->token);
            exit(1);
        }
        add_operator_from_expr(op1, output);
        pop_back(operators);
    }

    temp = init_ast(AST_EXPR);
    temp->nodes = output;
    list_push(expressions, temp);

    return expressions;

}

struct Ast * parser_parse_expr(struct Parser * parser) {
    struct Ast * node = init_ast(AST_EXPR);

    struct List * output = init_list(sizeof(struct Ast *));
    struct Deque * operators = init_deque(sizeof(struct Operator *));

    node->nodes = _parser_parse_expr(parser, output, operators);

    if (parser->prev->type == TOKEN_SEMI) {
        print_token("[Warning] Unnecessary semicolon\n{s}\n", parser->prev);
    }

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
