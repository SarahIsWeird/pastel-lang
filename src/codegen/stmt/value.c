//
// Created by sarah on 3/15/24.
//

#include "value.h"

#include "../casting.h"
#include "../expr/expr.h"

#include <stdio.h>

#include <llvm-c/Core.h>

typed_value_t *compile_return(compiler_t *compiler, expr_t *value_expr) {
    typed_value_t *value = compile_expr(compiler, value_expr, 0);
    if (value == NULL) return NULL;

    typed_value_t *return_value = malloc_s(typed_value_t);
    return_value->type = compiler->void_type;
    return_value->value = LLVMBuildRet(compiler->builder, value->value);
    return value;
}

typed_value_t *compile_assignment(compiler_t *compiler, assignment_stmt_data_t *data) {
    typed_value_t *value = compile_expr(compiler, data->value, 0);
    if (value == NULL) return NULL;

    variable_t *variable = find_variable(compiler, data->name);
    if (variable == NULL) {
        fprintf(stderr, "Unknown variable %ls!\n", data->name);
        return NULL;
    }

    if (!(variable->flags & VAR_IS_MUTABLE) && (variable->flags & VAR_IS_INITIALIZED)) {
        fprintf(stderr, "Can't assign to immutable variable %ls!\n", data->name);
        return NULL;
    }

    if (value->type != variable->type) {
        // We only do implicit type conversion for constant integers as of now.
        int is_value_int = (value->type->flags & TYPE_INT) != 0;
        int is_var_int = (variable->type->flags & TYPE_INT) != 0;
        if (!is_value_int || !is_var_int) {
            fprintf(stderr, "Can't do implicit type conversion between %ls and %ls!\n", value->type->name, variable->type->name);
            return NULL;
        }

        value = cast_to_int(compiler, value, variable->type);
    }

    variable->flags |= VAR_IS_INITIALIZED;

    typed_value_t *ret = malloc_s(typed_value_t);
    ret->type = compiler->void_type;
    ret->value = LLVMBuildStore(compiler->builder, value->value, variable->value);
    return ret;
}
