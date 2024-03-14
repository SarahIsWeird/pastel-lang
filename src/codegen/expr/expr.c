//
// Created by sarah on 3/15/24.
//

#include "expr.h"

#include "binop.h"
#include "value.h"
#include "if.h"
#include "call.h"

typed_value_t *compile_expr(compiler_t *compiler, expr_t *expr, int is_stmt) {
    switch (expr->expr_type) {
        case EXPR_INT:
            return compile_int_expr(compiler, (int_expr_t *) expr);
        case EXPR_BOOL:
            return compile_bool_expr(compiler, (bool_expr_t *) expr);
        case EXPR_VARIABLE:
            return compile_variable_expr(compiler, (variable_expr_t *) expr);
        case EXPR_BINARY:
            return compile_binary_expr(compiler, (binary_expr_t *) expr);
        case EXPR_CALL:
            return compile_call_expr(compiler, (call_expr_t *) expr);
        case EXPR_IF:
            return compile_if_expr(compiler, (if_expr_t *) expr, is_stmt);
        case EXPR_CAST:
            return compile_cast_expr(compiler, (cast_expr_t *) expr);
    }
}
