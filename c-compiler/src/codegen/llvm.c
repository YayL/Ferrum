#include "codegen/llvm.h"
#include "parser/types.h"

const char * llvm_type_to_llvm_type(Type type, struct AST * self_type) {
    switch (type.intrinsic) {
        case IUnknown:
            return "opaque";
        case INumeric:
        {
            Numeric_T num = type.value.numeric;
            if (num.type == NUMERIC_SIGNED || num.type == NUMERIC_UNSIGNED)
                return format("i{u}", num.width);
            else
                return format("f{u}", num.width);
        }
        case IStruct:
            return format("%struct.{s}", type.name);
        case IRef:
            return "ptr";
        case IArray:
        {
            Array_T array = type.value.array;
            return format("[{u} x {s}]", array.size, llvm_type_to_llvm_type(*array.basetype, self_type));
        }
        /* case ISelf: */
        /*     return llvm_type_to_llvm_type(self_type->value, NULL); */
        default:
            FATAL("Intrinsic type not implemented: {i}", type.intrinsic);
    }
}

const char * llvm_type_to_llvm_arg_type(Type type, struct AST * self_type) {
    switch (type.intrinsic) {
        case IStruct:
            return format("ptr byval(%struct.{s})", type.name);
        default:
            return llvm_type_to_llvm_type(type, self_type);
    }
}

const char * llvm_any_ast_to_llvm_type(struct AST * ast, struct AST * self_type) {
    return llvm_type_to_llvm_type(*ast_get_type_of(ast), self_type);
}

unsigned int llvm_get_register_of(struct AST * ast) {
    ASSERT1(ast->type == AST_VARIABLE);

    return ast->value.variable.reg;
}
