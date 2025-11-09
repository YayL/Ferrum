#include "parser/parser.h"

#include "parser/lexer.h"
#include "common/data/deque.h"
#include "tables/registry_manager.h"

Operator get_operator(SourceSpan span, struct Token token, enum OP_mode mode, char * enclosed_flag) {
    Operator op = str_to_operator(span, mode, enclosed_flag);

    if (op.key == OP_NOT_FOUND && mode == BINARY) {
        op = str_to_operator(span, UNARY_POST, enclosed_flag);
    }

    if (op.key == OP_NOT_FOUND) {
        FATAL("{s} operator '{s}' not found: {s}", mode == BINARY ? "Binary" : "Unary", string_init_from_source_span(span)._ptr, token_to_str(token));
        print_trace();
        exit(1);
    }

    return op;
}

void consume_add_operator(Operator op, Arena * arena, struct Parser * parser) {
    a_operator * operator = ast_allocate(ID_AST_OP, parser->current_scope_id);

    if (arena->size == 0) {
        FATAL("Invalid expression: Operator without valid operands");
    }

    operator->op = op;
    operator->right_id = ARENA_POP(arena, ID);

    if (op.mode == BINARY) {
        if (arena->size == 0) {
            FATAL("Invalid expression: Binary operator without two valid operands");
        }
        operator->left_id = ARENA_POP(arena, ID);
    }

    ARENA_APPEND(arena, operator->info.node_id);
}

// The parse_expr family of functions really need a refactor but they are a blackbox so I will not touch them for now
Arena _parser_parse_expr(struct Parser * parser, Arena * output, DEQUE_T(Operator) * operators, enum Operators EXIT_ON_KEY) {
    Arena expressions = arena_init(sizeof(struct AST *));

    a_operator * node_op = NULL;
    a_expression * node_expr = NULL;
    
    enum OP_mode mode = UNARY_PRE;
    struct Operator op1, op2;

    char flag;

    while (1) {
        switch (parser->lexer.tok.type) {
            case TOKEN_ID: {
                if (mode == BINARY) {
                    println("[Parser] Invalid expression token: {s}\n", token_to_str(parser->lexer.tok));
                    exit(1);
                }

                ID node_id = parser_parse_id(parser);

                ASSERT1(ID_IS(node_id, ID_AST_SYMBOL));
                a_symbol symbol = LOOKUP(node_id, a_symbol);

                if (!ID_IS_INVALID(symbol.node_id) && output->size != 0) {
                    ERROR("Invalid use of symbol type hinting");
                    exit(1);
                }

                ARENA_APPEND(output, node_id);
                mode = BINARY;
            } break;
            case TOKEN_INT: {
                ARENA_APPEND(output, parser_parse_int(parser));
                mode = BINARY;
            } break;
            case TOKEN_STRING_LITERAL: {
                ARENA_APPEND(output, parser_parse_string(parser));
                mode = BINARY;
            } break;
            case TOKEN_OP: {
                // enclosed flag is true if enclosed operator is the closing enclosing operator
                op1 = get_operator(parser->lexer.tok.span, parser->lexer.tok, mode, &flag);

                if (op1.enclosed == ENCLOSED) {
                    if (!flag) { // open enclosed operator
                        parser_eat(parser, parser->lexer.tok.type);
                        
                        DEQUE_T(Operator) deque = DEQUE_INIT(Operator);
                        Arena arena = arena_init(sizeof(ID));
                        DEQUE_PUSH_BACK(Operator, &deque, op1);

                        node_op = ast_allocate(ID_AST_OP, parser->current_scope_id);
                        node_expr = ast_allocate(ID_AST_EXPR, parser->current_scope_id);
                        node_op->op = op1;
 
                        node_expr->children = _parser_parse_expr(parser, &arena, &deque, -1);
                        arena_free(arena);
                        DEQUE_FREE(Operator, deque);
                        node_op->right_id = node_expr->info.node_id;
                    } else { // closing enclosed operator
                        while (op2 = DEQUE_BACK(Operator, operators), strcmp(op1.str, op2.str)) { // while not start version of this enclosed operator
                            if (op2.key == EXIT_ON_KEY) {
                                ASSERT(sizeof(op1.key) != operator_get_count(), "Possibly invalid EXIT_ON_KEY:");
                                goto exit;
                            }

                            if (operators->size == 1) {
                                println("[Parser] Unmatched enclosed operator: {s}", token_to_str(parser->lexer.tok));
                                exit(1);
                            }

                            consume_add_operator(op2, output, parser);
                            DEQUE_POP_BACK(Operator, operators);
                        }
                       
                        DEQUE_POP_BACK(Operator, operators);
                        parser_eat(parser, parser->lexer.tok.type);
                        goto exit;
                    }
                }

                op2 = DEQUE_BACK(Operator, operators);
                while (operators->size && (op2.enclosed != ENCLOSED &&
                        (op2.precedence < op1.precedence || (op1.precedence == op2.precedence && op2.associativity == LEFT))))
                {
                    consume_add_operator(op2, output, parser);
                    DEQUE_POP_BACK(Operator, operators);
                    op2 = DEQUE_BACK(Operator, operators);
                }
                
                if (op1.enclosed == ENCLOSED) {
                    if (op1.mode == BINARY) {
                        node_op->left_id = ARENA_POP(output, ID);
                    }

                    ARENA_APPEND(output, node_op->info.node_id);
                    mode = BINARY;
                    break;
                }

                mode = op1.mode == UNARY_POST 
                        ? BINARY 
                        : UNARY_PRE;
                parser_eat(parser, parser->lexer.tok.type);

                if (op1.key == ADDRESS_OF
                    && parser->lexer.tok.type == TOKEN_ID 
                    && id_is_equal(parser->lexer.tok.interner_id, keyword_get_intern_id(KEYWORD_MUT)))
                {
                    parser_eat(parser, TOKEN_ID);
                    op1 = operator_get(MUT_ADDRESS_OF);
                }

                if (op1.key == CAST) {
                    consume_add_operator(op1, output, parser);

                    ID cast_id = ARENA_GET(*output, output->size - 1, ID);
                    a_operator * cast_op = lookup(cast_id);
                    cast_op->type_id = parser_parse_type(parser);
                } else {
                    DEQUE_PUSH_BACK(Operator, operators, op1);
                }
            } break;
            case TOKEN_SEMI:
            case TOKEN_COMMA:
            {
                while (operators->size && (op2 = DEQUE_BACK(Operator, operators), op2.enclosed != ENCLOSED)) {
                    consume_add_operator(op2, output, parser);
                    DEQUE_POP_BACK(Operator, operators);
                }
                
                if (output->size == 1) {
                    ID temp_id = ARENA_POP(output, ID);
                    ARENA_APPEND(&expressions, temp_id);
                } else if (output->size != 1) {
                    FATAL("[Parser] Unprecedentent usage of expression separator: {s}", token_to_str(parser->lexer.tok));
                }

                mode = UNARY_PRE;
                parser_eat(parser, parser->lexer.tok.type);
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
                lexer_parse_operator(&parser->lexer);
                break;
            case TOKEN_LINE_BREAK:
                if (parser->previous_token.type == TOKEN_BACKSLASH) {
                    break;
                }
            case TOKEN_EOF:
            case TOKEN_LBRACE:
            case TOKEN_RBRACE:
                goto exit;
            default:
                FATAL("[Parser] Unrecognized token in expression: {s}", token_to_str(parser->lexer.tok));
        }
    }

exit: 

    // re-use flag to check if there has been an incorrectly placed operator
    flag = operators->size != 0;

    while (operators->size) {
        op1 = DEQUE_BACK(Operator, operators);
        if (op1.enclosed == ENCLOSED) {
            FATAL("[Parser] Unmatched enclosed operator near: {s}", token_to_str(parser->lexer.tok));
            exit(1);
        }
        consume_add_operator(op1, output, parser);
        DEQUE_POP_BACK(Operator, operators);
    }

    if (output->size == 1) {
        ID temp_id = ARENA_POP(output, ID);
        ARENA_APPEND(&expressions, temp_id);
    } else if (flag) {
        ERROR("Invalid expression; too many discarded expressions({i}) | {s}", output->size, token_to_str(parser->lexer.tok));
    }

    return expressions;

}

ID parser_parse_expr_exit_on(struct Parser * parser, enum Operators op) {
    a_expression * expr = ast_allocate(ID_AST_EXPR, parser->current_scope_id);

    Arena output = arena_init(sizeof(ID));
    DEQUE_T(Operator) operators = DEQUE_INIT(Operator);

    if (op != -1) {
        DEQUE_PUSH_FRONT(Operator, &operators, operator_get(op));
    }

    expr->children = _parser_parse_expr(parser, &output, &operators, op);

    arena_free(output);
    DEQUE_FREE(Operator, operators);

    if (parser->previous_token.type == TOKEN_SEMI) {
        WARN("Unnecessary semicolon: {s}", token_to_str(parser->previous_token));
    }

    return expr->info.node_id;
}

ID parser_parse_expr(struct Parser * parser) {
    return parser_parse_expr_exit_on(parser, -1);
}
