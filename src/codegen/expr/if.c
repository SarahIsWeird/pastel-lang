//
// Created by sarah on 3/15/24.
//

#include "if.h"

#include "expr.h"
#include "../utils.h"
#include "../stmt/stmt.h"

#include <stdio.h>

#include <llvm-c/Core.h>

static int can_if_be_expr(if_expr_t *expr) {
    ptr_list_t *then_stmts = expr->data->then_stmts;
    ptr_list_t *else_stmts = expr->data->else_stmts;

    // No else branch
    if (else_stmts == NULL) return 0;

    // Last statement in a branch has to be an expression statement for us to get a value from the branch.

    stmt_t *last_then_stmt = ptr_list_at(then_stmts, ptr_list_size(then_stmts) - 1);
    if (last_then_stmt->stmt_type != STMT_EXPR) return 0;

    stmt_t *last_else_stmt = ptr_list_at(else_stmts, ptr_list_size(else_stmts) - 1);
    if (last_else_stmt->stmt_type != STMT_EXPR) return 0;

    return 1;
}

typed_value_t *compile_if_expr(compiler_t *compiler, if_expr_t *if_expr, int is_stmt) {
    int can_be_expr = can_if_be_expr(if_expr);
    int is_expr = !is_stmt && can_be_expr;

    // Do sanity check up front
    if (!is_stmt && !can_be_expr) {
        fprintf(stderr, "An if statement was used as an expression, but it doesn't fit the right form!\n");
        return NULL;
    }

    ptr_list_t *then_stmts = if_expr->data->then_stmts;
    ptr_list_t *else_stmts = if_expr->data->else_stmts;

    LLVMValueRef current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(compiler->builder));

    LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(compiler->context, current_function, "then");
    LLVMBasicBlockRef cont_block = LLVMCreateBasicBlockInContext(compiler->context, "cont");
    LLVMBasicBlockRef else_block = cont_block;

    if (else_stmts != NULL) {
        else_block = LLVMCreateBasicBlockInContext(compiler->context, "else");
    }

    typed_value_t *condition = compile_expr(compiler, if_expr->data->condition, 0);
    if (condition == NULL) return NULL;
    if (condition->type != compiler->bool_type) {
        fprintf(stderr, "If conditions must be boolean!\n");
        return NULL;
    }

    LLVMBuildCondBr(compiler->builder, condition->value, then_block, else_block);
    LLVMPositionBuilderAtEnd(compiler->builder, then_block);

    size_t i;
    typed_value_t *then_value;

    for (i = 0; i < ptr_list_size(then_stmts); i++) {
        stmt_t *stmt = (stmt_t *) ptr_list_at(then_stmts, i);
        then_value = compile_stmt(compiler, stmt);
        if (then_value == NULL) return NULL;

        if (LLVMIsAReturnInst(then_value->value)) break;
    }

    /*
     * We can't have the if_value (so, the return value of compile_if_expr() be the then_value, since if the last
     * statement in the then block is a return and there's no else branch, we'd return a value ref pointing to a
     * return, which in turn messes up code upstream.
     */
    typed_value_t *if_value = malloc_s(typed_value_t);
    if_value->type = compiler->void_type;

    if (!LLVMIsAReturnInst(then_value->value)) {
        if_value->value = LLVMBuildBr(compiler->builder, cont_block);
    } else {
        if_value->value = condition->value;
    }

    then_block = LLVMGetInsertBlock(compiler->builder); // We might be in a different block than where we started.

    typed_value_t *else_value = NULL;
    // FIXME: empty block would deref null
    if (else_stmts != NULL) {
        LLVMAppendExistingBasicBlock(current_function, else_block);
        LLVMPositionBuilderAtEnd(compiler->builder, else_block);

        for (i = 0; i < ptr_list_size(else_stmts); i++) {
            stmt_t *stmt = (stmt_t *) ptr_list_at(else_stmts, i);
            else_value = compile_stmt(compiler, stmt);
            if (else_value == NULL) return NULL;

            if (LLVMIsAReturnInst(else_value->value)) break;
        }

        if (!LLVMIsAReturnInst(else_value->value)) {
            LLVMBuildBr(compiler->builder, cont_block);
        }

        else_block = LLVMGetInsertBlock(compiler->builder);
    }

    /*
     * Same as before, but we omit the cont_block if there's early returns in both branches.
     */
    if (LLVMIsAReturnInst(then_value->value) && LLVMIsAReturnInst(else_value->value)) {
        return if_value;
    }

    LLVMAppendExistingBasicBlock(current_function, cont_block);
    LLVMPositionBuilderAtEnd(compiler->builder, cont_block);

    if (!is_expr) {
        // We don't need any PHI node stuff, we can just return the jmp from the then branch.
        return if_value;
    }

    if (then_value->type != else_value->type) {
        fprintf(stderr, "Types must match for both branches when using if as an expression!\n");
        return NULL;
    }

    LLVMValueRef incoming_values[] = { then_value->value, else_value->value };
    LLVMBasicBlockRef incoming_blocks[] = { then_block,else_block };
    LLVMValueRef phi = LLVMBuildPhi(compiler->builder, then_value->type->llvm_type, "if_value");

    LLVMAddIncoming(phi, incoming_values, incoming_blocks, 2);

    if_value->type = then_value->type;
    if_value->value = phi;

    return if_value;
}
