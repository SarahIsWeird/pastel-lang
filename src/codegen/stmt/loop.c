//
// Created by sarah on 3/15/24.
//

#include "loop.h"

#include "stmt.h"
#include "../expr/expr.h"
#include "../utils.h"

#include <stdio.h>

#include <llvm-c/Core.h>

typed_value_t *compile_while(compiler_t *compiler, while_stmt_data_t *data) {
    LLVMValueRef current_function = LLVMGetBasicBlockParent(LLVMGetInsertBlock(compiler->builder));

    LLVMBasicBlockRef cond_block = LLVMAppendBasicBlockInContext(compiler->context, current_function, "loop_cond");
    LLVMBasicBlockRef loop_body = LLVMCreateBasicBlockInContext(compiler->context, "loop_body");
    LLVMBasicBlockRef cont_block = LLVMCreateBasicBlockInContext(compiler->context, "loop_cont");

    LLVMBuildBr(compiler->builder, cond_block);
    LLVMPositionBuilderAtEnd(compiler->builder, cond_block);

    typed_value_t *condition = compile_expr(compiler, data->condition, 0);
    if (condition == NULL) return NULL;
    if (condition->type != compiler->bool_type) {
        fprintf(stderr, "Expected boolean type for while condition, but got %ls!\n", condition->type->name);
        return NULL;
    }

    typed_value_t *ret = malloc_s(typed_value_t);
    ret->type = compiler->void_type;
    ret->value = LLVMBuildCondBr(compiler->builder, condition->value, loop_body, cont_block);

    LLVMAppendExistingBasicBlock(current_function, loop_body);
    LLVMPositionBuilderAtEnd(compiler->builder, loop_body);

    size_t i;
    for (i = 0; i < ptr_list_size(data->body); i++) {
        stmt_t *stmt = (stmt_t *) ptr_list_at(data->body, i);
        typed_value_t *value = compile_stmt(compiler, stmt);
        if (value == NULL) return NULL;

        if (stmt->stmt_type == STMT_RETURN) {
            free(ret);
            ret = value;
            break;
        }
    }

    if (!LLVMIsAReturnInst(ret->value)) {
        LLVMBuildBr(compiler->builder, cond_block);
        LLVMAppendExistingBasicBlock(current_function, cont_block);
        LLVMPositionBuilderAtEnd(compiler->builder, cont_block);
    }

    return ret;
}
