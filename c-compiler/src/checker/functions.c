#include "checker/functions.h"

#include "checker/typing.h"
#include "parser/AST.h"
#include "tables/registry_manager.h"

void resolve_function_from_call(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * op = lookup(node_id);

    ID left_id = op->left_id, right_id = op->right_id;
    ASSERT1(ID_IS(left_id, ID_AST_VARIABLE));
    a_variable function_variable = LOOKUP(node_id, a_variable);

    ID args_type_id = ast_to_type(right_id);
    FRResult result = frsolver_solve(frsolver_init(function_variable.name_id, args_type_id, function_variable.info.scope_id));

    if (ID_IS_INVALID(result.function_id)) {
        ERROR("Function {s}({s}) is not defined", op->op.str, type_to_str(args_type_id));
        exit(1);
    }

    op->definition.function_id = result.function_id;
    op->definition.substitution = result.substitutions;
}

void resolve_function_from_operator(ID node_id) {
    ASSERT1(ID_IS(node_id, ID_AST_OP));
    a_operator * op = lookup(node_id);

    ID left_id = op->left_id, right_id = op->right_id;

    Arena tuple_types = arena_init(sizeof(ID));

    if (op->op.mode == BINARY) {
        ARENA_APPEND(&tuple_types, ast_get_type_of(left_id));
    }
    ARENA_APPEND(&tuple_types, ast_get_type_of(right_id));

    FRResult result = frsolver_solve(frsolver_init(operator_get_intern_id(op->op.key), INVALID_ID, op->info.scope_id));

    if (ID_IS_INVALID(result.function_id)) {
        Tuple_T * temp = type_allocate(ID_AST_OP);
        temp->types = tuple_types;
        ERROR("Operator '{s}'({s}) is not defined for {s}", op->op.str, operator_get_runtime_name(op->op.key), type_to_str(temp->info.type_id));
        exit(1);
    }

    op->definition.function_id = result.function_id;
    op->definition.substitution = result.substitutions;
}

ID get_member_function(const char * member_name, ID args_type_id, ID return_type_id, ID scope_id) {
    ASSERT1(!ID_IS_INVALID(args_type_id));
    ASSERT1(!ID_IS_INVALID(return_type_id));


    return INVALID_ID;
}
