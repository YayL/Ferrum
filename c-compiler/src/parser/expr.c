#include "fmt.h"
#include "parser/parser.h"

#include "parser/lexer.h"
#include "common/deque.h"
#include "codegen/AST.h"
#include "parser/operators.h"
#include "common/list.h"
#include "common/logger.h"
#include "common/sourcespan.h"
#include "common/macro.h"

struct Operator * get_operator(SourceSpan span, struct Token * token, enum OP_mode mode, char * enclosed_flag) {
    struct Operator * op = malloc(sizeof(struct Operator));
    *op = str_to_operator(span.start, mode, enclosed_flag);

    if (op->key == OP_NOT_FOUND && mode == BINARY)
        *op = str_to_operator(span.start, UNARY_POST, enclosed_flag);

    if (op->key == OP_NOT_FOUND) {
        ERROR("{s} operator '{s}' not found: {s}", mode == BINARY ? "Binary" : "Unary", span.start, token_to_str(*token));
        exit(1);
    }

    return op;
}

void consume_add_operator(struct Operator * op, struct List * list, struct Parser * parser) {
    struct AST * ast = init_ast(AST_OP, parser->current_scope);
    a_op * operator = &ast->value.operator;

    if (list->size == 0) {
        FATAL("Invalid expression: Operator without valid operands");
    }

    operator->op = op;
    operator->right = list_at(list, -1);
    list_pop(list);

    if (op->mode == BINARY) {
        if (list->size == 0) {
            FATAL("Invalid expression: Binary operator without two valid operands");
        }
        operator->left = list_at(list, -1);
        list_pop(list);
    }

    list_push(list, ast);
}

struct List * _parser_parse_expr(struct Parser * parser, struct List * output, struct Deque * operators, enum Operators EXIT_ON_KEY) {
    struct AST * node, * temp;
    struct List * expressions = init_list(sizeof(struct AST *));
    
    enum OP_mode mode = UNARY_PRE;
    struct Operator * op1, * op2;

    char flag;

    while (1) {
        switch (parser->token->type) {
            case TOKEN_ID:
            {
                if (mode == BINARY) {
                    println("[Parser] Invalid expression token: {s}\n", token_to_str(*parser->token));
                    exit(1);
                }

                list_push(output, parser_parse_id(parser));
                mode = BINARY;
            } break;
            case TOKEN_INT:
            {
                list_push(output, parser_parse_int(parser));

                mode = BINARY;
            } break;
            case TOKEN_STRING_LITERAL:
            {
                list_push(output, parser_parse_string(parser));
                mode = BINARY;
            } break;
            case TOKEN_OP:
            {
                // enclosed flag is true if enclosed operator is the closing enclosing operator
                op1 = get_operator(parser->token->span, parser->token, mode, &flag);

                if (op1->enclosed == ENCLOSED) {
                    if (!flag) { // open enclosed operator
                        parser_eat(parser, parser->token->type);
                        
                        struct Deque * temp_d = init_deque(sizeof(struct Operator *));
                        struct List * temp_l = init_list(sizeof(struct AST *));
                        push_back(temp_d, op1);

                        node = init_ast(AST_OP, parser->current_scope);
                        temp = init_ast(AST_EXPR, parser->current_scope);
                        node->value.operator.op = op1;
 
                        temp->value.expression.children = _parser_parse_expr(parser, temp_l, temp_d, -1);
                        node->value.operator.right = temp;
                    } else { // closing enclosed operator
                        while (strcmp(op1->str, (op2 = deque_back(operators))->str)) { // while not start version of this enclosed operator
                            if (op2->key == EXIT_ON_KEY) {
                                ASSERT(sizeof(op1->key) != (sizeof(op_conversion) / sizeof(op_conversion[0])), "Possibly invalid EXIT_ON_KEY:");
                                goto exit;
                            }
                            if (operators->size == 1) {
                                println("[Parser] Unmatched enclosed operator: {s}", token_to_str(*parser->token));
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
                
                if (op1->enclosed == ENCLOSED) {
                    if (op1->mode == BINARY) {
                        node->value.operator.left = list_at(output, -1);
                        list_pop(output);
                    }

                    list_push(output, node);
                    mode = BINARY;
                    break;
                }

                mode = op1->mode == UNARY_POST 
                        ? BINARY 
                        : UNARY_PRE;
                parser_eat(parser, parser->token->type);

                if (op1->key == CAST || op1->key == BIT_CAST) {
                    consume_add_operator(op1, output, parser);

                    struct AST * cast_ast = list_at(output, -1);
                    ALLOC(cast_ast->value.operator.type);
                    *cast_ast->value.operator.type = parser_parse_type(parser);
                } else {
                    push_back(operators, op1);
                }
            } break;
            case TOKEN_SEMI:
            case TOKEN_COMMA:
            {
                while (operators->size && (op2 = deque_back(operators))->enclosed != ENCLOSED) {
                    consume_add_operator(op2, output, parser);
                    pop_back(operators);
                }
                
                if (output->size == 1) {
                    list_push(expressions, list_at(output, -1));
                    output = init_list(sizeof(struct AST *));
                }else if (output->size != 1) {
                    FATAL("[Parser] Unprecedentent usage of expression separator: {s}", token_to_str(*parser->token));
                }

                mode = UNARY_PRE;
                parser_eat(parser, parser->token->type);
            } break;
            case TOKEN_SLASH:
            case TOKEN_PLUS:
            case TOKEN_MINUS:
            case TOKEN_EQUAL:
            case TOKEN_LT:
            case TOKEN_GT:
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
            case TOKEN_LBRACKET:
            case TOKEN_RBRACKET:
                lexer_parse_operator(parser->lexer);
                break;
            case TOKEN_LINE_BREAK:
                if (parser->prev.type == TOKEN_BACKSLASH) {
                    break;
                }
            case TOKEN_EOF:
            case TOKEN_LBRACE:
            case TOKEN_RBRACE:
                goto exit;
            default:
                FATAL("[Parser] Unrecognized token in expression: {s}", token_to_str(*parser->token));
        }
    }

exit: 

    // re-use flag to check if there has been an incorrectly placed operator
    flag = operators->size != 0;

    while (operators->size) {
        op1 = deque_back(operators);
        if (op1->enclosed == ENCLOSED) {
            FATAL("[Parser] Unmatched enclosed operator near: {s}", token_to_str(*parser->token));
            exit(1);
        }
        consume_add_operator(op1, output, parser);
        pop_back(operators);
    }

    if (output->size == 1) {
        list_push(expressions, list_at(output, 0));
    } else if (flag) {
        ERROR("Invalid expression; too many discarded expressions({i}) | {s}", output->size, token_to_str(*parser->token));
    }

    return expressions;

}

struct AST * parser_parse_expr_exit_on(struct Parser * parser, enum Operators op) {
    struct AST * ast = init_ast(AST_EXPR, parser->current_scope);

    struct List * output = init_list(sizeof(struct AST *));
    struct Deque * operators = init_deque(sizeof(struct Operator *));

    if (op != -1) {
        struct Operator * temp_operator = malloc(sizeof(struct Operator));
        for (int i = 0; i < (sizeof(op_conversion) / sizeof(struct Operator)); ++i) {
            if (op == op_conversion[i].key) {
                *temp_operator = op_conversion[i];
                break;
            }
        }
        push_front(operators, temp_operator);
    }

    ast->value.expression.children = _parser_parse_expr(parser, output, operators, op);

    if (parser->prev.type == TOKEN_SEMI) {
        WARN("Unnecessary semicolon: {s}", token_to_str(parser->prev));
    }

    return ast;
}

struct AST * parser_parse_expr(struct Parser * parser) {
    return parser_parse_expr_exit_on(parser, -1);
}
