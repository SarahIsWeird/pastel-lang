//
// Created by sarah on 3/14/24.
//

#include "binop.h"

#include "expr.h"
#include "../casting.h"

#include <llvm-c/Core.h>
#include <stdio.h>

static typed_value_t *create_arithmetic_int_binop_inst(LLVMBuilderRef builder, type_t *int_type, wchar_t *op, LLVMValueRef lhs, LLVMValueRef rhs) {
    typed_value_t *value = malloc_s(typed_value_t);

    if (!wcscmp(L"+", op)) {
        value->type = int_type;
        value->value = LLVMBuildAdd(builder, lhs, rhs, "add_tmp");
        return value;
    }

    if (!wcscmp(L"-", op)) {
        value->type = int_type;
        value->value = LLVMBuildSub(builder, lhs, rhs, "sub_tmp");
        return value;
    }

    if (!wcscmp(L"*", op)) {
        value->type = int_type;
        value->value = LLVMBuildMul(builder, lhs, rhs, "mul_tmp");
        return value;
    }

    if (!wcscmp(L"/", op)) {
        value->type = int_type;
        value->value = LLVMBuildSDiv(builder, lhs, rhs, "mul_tmp");
        return value;
    }

    free(value);
    return NULL;
}

static typed_value_t *create_comp_int_binop_inst(compiler_t *compiler, type_t *int_type, wchar_t *op, LLVMValueRef lhs, LLVMValueRef rhs) {
    typed_value_t *value = malloc_s(typed_value_t);

    int is_signed = (int_type->flags & TYPE_SIGNED) != 0;

    if (!wcscmp(L"==", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntEQ, lhs, rhs, "eq_tmp");
        return value;
    }

    if (!wcscmp(L"!=", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntNE, lhs, rhs, "ne_tmp");
        return value;
    }

    if (!wcscmp(L"<", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, is_signed ? LLVMIntSLT : LLVMIntULT, lhs, rhs, "lt_tmp");
        return value;
    }

    if (!wcscmp(L"<=", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, is_signed ? LLVMIntSLE : LLVMIntULE, lhs, rhs, "le_tmp");
        return value;
    }

    if (!wcscmp(L">", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, is_signed ? LLVMIntSGT : LLVMIntUGT, lhs, rhs, "gt_tmp");
        return value;
    }

    if (!wcscmp(L">=", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, is_signed ? LLVMIntSGE : LLVMIntUGE, lhs, rhs, "ge_tmp");
        return value;
    }

    free(value);
    return NULL;
}

static typed_value_t *create_int_binop_inst(compiler_t *compiler, type_t *int_type, wchar_t *op, LLVMValueRef lhs, LLVMValueRef rhs) {
    typed_value_t *value = create_arithmetic_int_binop_inst(compiler->builder, int_type, op, lhs, rhs);
    if (value != NULL) return value;

    value = create_comp_int_binop_inst(compiler, int_type, op, lhs, rhs);
    if (value != NULL) return value;

    return NULL;
}

static typed_value_t *create_bool_cmp_inst(compiler_t *compiler, wchar_t *op, LLVMValueRef lhs, LLVMValueRef rhs) {
    typed_value_t *value = malloc_s(typed_value_t);
    value->type = compiler->bool_type;

    if (!wcscmp(L"==", op)) {
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntEQ, lhs, rhs, "eq_tmp");
        return value;
    }

    if (!wcscmp(L"!=", op)) {
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntNE, lhs, rhs, "ne_tmp");
        return value;
    }

    free(value);
    return NULL;
}

typed_value_t *compile_binary_expr(compiler_t *compiler, binary_expr_t *binary_expr) {
    typed_value_t *lhs = compile_expr(compiler, binary_expr->data->lhs, 0);
    if (lhs == NULL) return NULL;

    typed_value_t *rhs = compile_expr(compiler, binary_expr->data->rhs, 0);
    if (rhs == NULL) {
        free(lhs);
        return NULL;
    }

    do_type_coercion(compiler, &lhs, &rhs);

    if (lhs->type != rhs->type) {
        fprintf(stderr, "Types in binary don't match! (%ls and %ls)\n", lhs->type->name, rhs->type->name);

        free(lhs);
        free(rhs);
        return NULL;
    }

    wchar_t *op = binary_expr->data->op;

    typed_value_t *value = NULL;
    if (lhs->type == compiler->bool_type) {
        value = create_bool_cmp_inst(compiler, op, lhs->value, rhs->value);
    } else if ((lhs->type->flags & TYPE_INT) != 0) {
        value = create_int_binop_inst(compiler, lhs->type, op, lhs->value, rhs->value);
    }

    if (value == NULL) {
        fprintf(stderr, "Unknown binary operator '%ls' for type %ls!\n", op, lhs->type->name);
        return NULL;
    }

    free(lhs);
    free(rhs);

    return value;
}
