#include "parser/parser.h"

// TODO:
//
// Make all operators have a left and right side (well unless they are unary of course)
// so that it is easier to recognize LHS and RHS for assignments[??]

struct Operator * get_operator(const char * str, struct Token * token, enum OP_mode mode, char * enclosed_flag) {
    struct Operator * op = malloc(sizeof(struct Operator));
    *op = str_to_operator(str, mode, enclosed_flag);
    if (op->key == OP_NOT_FOUND && mode == BINARY)
        *op = str_to_operator(str, UNARY_POST, enclosed_flag);

    if (op->key == OP_NOT_FOUND) {
        print("[Parser]: {s} operator '{s}' not found: ", mode == BINARY ? "Binary" : "Unary", str);
        print_token("{s}\n", token);
        exit(1);
    }

    return op;
}

void consume_add_operator(struct Operator * op, struct List * list, struct Parser * parser) {
    struct Ast * ast = init_ast(AST_OP, parser->current_scope);
    a_op * operator = ast->value;
    
    if (list->size == 0) {
        logger_log("Invalid expression: Operator without valid operands", PARSER, ERROR);
        exit(1);
    }

    operator->op = op;
    operator->right = list_at(list, -1);
    list_pop(list);

    if (op->mode == BINARY) {
        if (list->size == 0) {
            logger_log("Invalid expression: Binary operator without two valid operands", PARSER, ERROR);
            exit(1);
        }
        operator->left = list_at(list, -1);
        list_pop(list);
    }

    list_push(list, ast);
}

struct List * _parser_parse_expr(struct Parser * parser, struct List * output, struct Deque * operators) {
    struct Ast * node, * temp;
    struct List * expressions = init_list(sizeof(struct Ast *));
    
    enum OP_mode mode = UNARY_PRE;
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

                list_push(output, parser_parse_id(parser));
                mode = BINARY;
                break;
            }
            case TOKEN_INT:
            {
                list_push(output, parser_parse_int(parser));

                mode = BINARY;
                break;
            }
            case TOKEN_STRING_LITERAL:
            {
                list_push(output, parser_parse_string(parser));
                mode = BINARY;
            } break;
            case TOKEN_OP:
            {
_TOKEN_OPERATORS:
                op1 = get_operator(parser->token->value, parser->token, mode, &enclosed_flag);

                if (op1->enclosed == ENCLOSED) {
                    if (!enclosed_flag) {
                        parser_eat(parser, parser->token->type);
                        
                        struct Deque * temp_d = init_deque(sizeof(struct Operator *));
                        struct List * temp_l = init_list(sizeof(struct Ast *));
                        push_back(temp_d, op1);

                        node = init_ast(AST_OP, parser->current_scope);
                        temp = init_ast(AST_EXPR, parser->current_scope);
                        ((a_op *) node->value)->op = op1;

                        if (op1->mode == BINARY) {
                            ((a_op *) node->value)->left = list_at(output, -1);
                            list_pop(output);
                        }
                        
                        ((a_expr *) temp->value)->children = _parser_parse_expr(parser, temp_l, temp_d);
                        ((a_op *) node->value)->right = temp;
                        
                        list_push(output, node);
                        
                        mode = BINARY;
                        break;
                    } else {
                        while (strcmp(op1->str, (op2 = deque_back(operators))->str)) {
                            if (operators->size == 1) {
                                print_token("[Parser] Unmatched enclosed operator: {s}\n", parser->token);
                                exit(1);
                            }
                            consume_add_operator(op2, output, parser);
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
                    consume_add_operator(op2, output, parser);
                    pop_back(operators);
                    op2 = deque_back(operators);
                }

                push_back(operators, op1);

                mode = op1->mode == UNARY_POST ? BINARY : UNARY_PRE;
                parser_eat(parser, parser->token->type);
                break;
            }
            case TOKEN_SEMI:
            case TOKEN_COMMA:
            {
                while (operators->size && (op2 = deque_back(operators))->enclosed != ENCLOSED) {
                    consume_add_operator(op2, output, parser);
                    pop_back(operators);
                }
                
                if (output->size == 0) {
                    list_push(expressions, NULL);
                }else if (output->size != 1) {
                    logger_log("Uhoh it appears that there is an issue with commas?", PARSER, ERROR);
                    exit(1);
                } else {
                    list_push(expressions, list_at(output, -1));
                    output = init_list(sizeof(struct Ast *));
                }

                mode = UNARY_PRE;
                parser_eat(parser, parser->token->type);
            } break;
            case TOKEN_SLASH:
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_EQUAL:
            case TOKEN_LESS_THAN:
            case TOKEN_GREATER_THAN:
            case TOKEN_LPAREN:
            case TOKEN_RPAREN:
            case TOKEN_COLON:
            case TOKEN_ASTERISK:
            case TOKEN_CARET:
            case TOKEN_AMPERSAND:
            case TOKEN_TILDA:
            case TOKEN_DOT:
            case TOKEN_PERCENT:
            case TOKEN_EXCLAMATION_MARK:
            case TOKEN_QUESTION_MARK:
            case TOKEN_VERTICAL_LINE:
                lexer_parse_operator(parser->lexer);
                break;
            case TOKEN_LINE_BREAK:
                if (parser->prev->type == TOKEN_BACKSLASH)
                    break;
            case TOKEN_EOF:
            case TOKEN_LBRACKET:
            case TOKEN_RBRACKET:
                goto exit;
            default:
                print_token("[Parser]: Unrecognized token in expression\n{s}\n", parser->token);
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
        consume_add_operator(op1, output, parser);
        pop_back(operators);
    }

    if (output->size != 1) {
        //print_ast_tree(list_at(output, 0));
        logger_log(format("Invalid expression: {i}", output->size), PARSER, ERROR);
        exit(1);
    } else {
        list_push(expressions, list_at(output, 0));
    }

    return expressions;

}

struct Ast * parser_parse_expr(struct Parser * parser) {
    struct Ast * ast = init_ast(AST_EXPR, parser->current_scope);

    struct List * output = init_list(sizeof(struct Ast *));
    struct Deque * operators = init_deque(sizeof(struct Operator *));

    ((a_expr *) ast->value)->children = _parser_parse_expr(parser, output, operators);

    if (parser->prev->type == TOKEN_SEMI) {
        print_token("[Warning] Unnecessary semicolon\n{s}\n", parser->prev);
    }

    return ast;
}
