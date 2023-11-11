#include "codegen/llvm.h"

const char * llvm_type_to_llvm_type(Type * type) {
    switch (type->intrinsic) {
        case INumeric:
            return type_to_str(type);
        case IStruct:
            return format("%struct.{s}", type->name);
        case IRef:
            return "ptr";
        case IArray:
        {
            Array_T * array = type->ptr;
            return format("[{u} x {s}]", array->size, llvm_type_to_llvm_type(array->basetype));
        }
        default:
            logger_log(format("Intrinsic type not implemented: {i}", type->intrinsic), IR, ERROR);
            exit(1);
    }
}

const char * llvm_ast_type_to_llvm_type(struct Ast * ast) {
    return llvm_type_to_llvm_type(ast->value);
}

const char * llvm_ast_type_to_llvm_arg_type(struct Ast * ast) {
    Type * type = ast->value;
    switch (type->intrinsic) {
        case IStruct:
            return format("ptr byval(%struct.{s})", type->name);
        default:
            return llvm_type_to_llvm_type(type);
    }
}
