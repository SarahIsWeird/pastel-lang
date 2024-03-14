//
// Created by sarah on 3/15/24.
//

#include "call.h"

#include "expr.h"
#include "../utils.h"

#include <stdio.h>

#include <llvm-c/Core.h>

typed_value_t *compile_call_expr(compiler_t *compiler, call_expr_t *call_expr) {
    function_t *callee = find_function_by_name(compiler, call_expr->data->callee_name);
    if (callee == NULL) {
        fprintf(stderr, "Unknown function %ls!\n", call_expr->data->callee_name);
        return NULL;
    }

    ptr_list_t *call_args = call_expr->data->arguments;

    if (LLVMCountParams(callee->function) != ptr_list_size(call_args)) {
        fprintf(
                stderr,
                "Expected %lu arguments but got %u in call to %ls!\n",
                ptr_list_size(call_args),
                LLVMCountParams(callee->function),
                call_expr->data->callee_name
        );
        return NULL;
    }

    size_t i;
    ptr_list_t *args = ptr_list_new();
    for (i = 0; i < ptr_list_size(call_args); i++) {
        expr_t *expr = ptr_list_at(call_args, i);

        typed_value_t *expr_value = compile_expr(compiler, expr, 0);
        if (expr_value == NULL) return NULL;

        annotated_typed_arg_t *callee_arg = (annotated_typed_arg_t *) ptr_list_at(callee->prototype->arguments, i);
        if (expr_value->type != callee_arg->type) {
            fprintf(
                    stderr,
                    "Expected type %ls for arg %lu in call to %ls but got value of %ls!\n",
                    callee_arg->type->name,
                    i,
                    callee->prototype->name,
                    expr_value->type->name
            );
            return NULL;
        }

        ptr_list_push(args, expr_value->value);
        free(expr_value);
    }

    // If the callee returns void, it's obviously illegal to save the result.
    const char *tmp_name = callee->prototype->return_type == compiler->void_type ? "" : "call_tmp";

    typed_value_t *return_value = malloc_s(typed_value_t);
    return_value->type = callee->prototype->return_type;
    return_value->value = LLVMBuildCall2(
            compiler->builder,
            callee->type,
            callee->function,
            (LLVMValueRef *) ptr_list_raw(args),
            ptr_list_size(args),
            tmp_name
    );

    ptr_list_free(args);
    return return_value;
}
