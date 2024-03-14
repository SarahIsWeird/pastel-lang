//
// Created by sarah on 3/15/24.
//

#include "value.h"

#include "expr.h"
#include "../casting.h"

#include <llvm-c/Core.h>
#include <stdio.h>

typed_value_t *compile_int_expr(compiler_t *compiler, int_expr_t *int_expr) {
    typed_value_t *value = malloc_s(typed_value_t);
    value->type = compiler->int32_type;
    value->value = LLVMConstInt(compiler->int32_type->llvm_type, int_expr->data, 0);
    return value;
}

typed_value_t *compile_bool_expr(compiler_t *compiler, bool_expr_t *bool_expr) {
    typed_value_t *value = malloc_s(typed_value_t);
    value->type = compiler->bool_type;
    value->value = LLVMConstInt(compiler->bool_type->llvm_type, bool_expr->data, 0);
    return value;
}

typed_value_t *compile_variable_expr(compiler_t *compiler, variable_expr_t *variable_expr) {
    variable_t *variable = NULL;

    int i;
    for (i = 0; i < ptr_list_size(compiler->variables); i++) {
        variable_t *variable_to_check = (variable_t *) ptr_list_at(compiler->variables, i);
        if (!wcscmp(variable_to_check->name, variable_expr->name)) {
            variable = variable_to_check;
            break;
        }
    }

    if (variable == NULL) {
        fprintf(stderr, "Unknown variable %ls!\n", variable_expr->name);
        return NULL;
    }

    typed_value_t *value = malloc_s(typed_value_t);
    value->type = variable->type;

    if (variable->flags & VAR_IS_PARAM) {
        value->value = variable->value;
        return value;
    }

    value->value = LLVMBuildLoad2(compiler->builder, variable->type->llvm_type, variable->value, "load_tmp");
    return value;
}

typed_value_t *compile_cast_expr(compiler_t *compiler, cast_expr_t *expr) {
    typed_value_t *value = compile_expr(compiler, expr->data->value, 0);
    if (value == NULL) return NULL;

    type_t *type = find_type(compiler, expr->data->type);
    if (type == NULL) {
        fprintf(stderr, "Unknown type %ls!\n", expr->data->type);
        return NULL;
    }

    return cast_value(compiler, value, type);
}
