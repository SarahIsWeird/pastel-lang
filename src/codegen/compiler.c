//
// Created by sarah on 3/12/24.
//

#include "compiler.h"
#include "../parser/ast.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/Utils.h>
#include <string.h>

#define malloc_s(t) ((t *) malloc(sizeof(t)))

typedef struct type_t {
    wchar_t *name;
    LLVMTypeRef llvm_type;
} type_t;

typedef struct typed_value_t {
    type_t *type;
    LLVMValueRef value;
} typed_value_t;

typedef struct variable_t {
    wchar_t *name;
    type_t *type;
    LLVMValueRef value;
    variable_flags_t flags;
} variable_t;

typedef struct annotated_typed_arg_t {
    wchar_t *name;
    type_t *type;
} annotated_typed_arg_t;

typedef struct annotated_prototype_t {
    wchar_t *name;
    type_t *return_type;
    ptr_list_t *arguments; // List<annotated_typed_arg_t *>
    int is_extern;
} annotated_prototype_t;

typedef struct function_t {
    annotated_prototype_t  *prototype;
    LLVMTypeRef type;
    LLVMValueRef function;
} function_t;

struct compiler_t {
    LLVMModuleRef module;
    LLVMContextRef context;
    LLVMBuilderRef builder;

    ptr_list_t *variables;
    ptr_list_t *functions;
    ptr_list_t *top_level_statements;
    ptr_list_t *types;

    type_t *void_type;
    type_t *bool_type;
    type_t *int32_type;

    compiler_opt_level_t opt_level;
    LLVMPassManagerRef function_pm;
};

static typed_value_t *compile_expr(compiler_t *compiler, expr_t *expr, int is_stmt);
static typed_value_t *compile_stmt(compiler_t *compiler, stmt_t *stmt);

static char wcstombs_buffer[512];
static char *to_mbs(const wchar_t *str) {
    if (wcstombs(wcstombs_buffer, str, 512) == 512) {
        wcstombs_buffer[511] = 0;
    }

    return wcstombs_buffer;
}

static type_t *create_type(wchar_t *name, LLVMTypeRef llvm_type) {
    type_t *type = malloc_s(type_t);
    type->name = name;
    type->llvm_type = llvm_type;
    return type;
}

static type_t *find_type(compiler_t *compiler, const wchar_t *name) {
    if (name == NULL) return compiler->void_type;

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->types); i++) {
        type_t *type = (type_t *) ptr_list_at(compiler->types, i);
        if (!wcscmp(name, type->name)) return type;
    }

    return NULL;
}

static variable_t *find_variable(compiler_t *compiler, const wchar_t *name) {
    if (name == NULL) return NULL;

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->variables); i++) {
        variable_t *variable = (variable_t *) ptr_list_at(compiler->variables, i);
        if (!wcscmp(name, variable->name)) return variable;
    }

    return NULL;
}

static function_t *find_function_by_name(compiler_t *compiler, const wchar_t *name) {
    size_t i;
    for (i = 0; i < ptr_list_size(compiler->functions); i++) {
        function_t *function = (function_t *) ptr_list_at(compiler->functions, i);
        if (!wcscmp(name, function->prototype->name)) return function;
    }

    return NULL;
}

static typed_value_t *compile_int_expr(compiler_t *compiler, int_expr_t *int_expr) {
    typed_value_t *value = malloc_s(typed_value_t);
    value->type = compiler->int32_type;
    value->value = LLVMConstInt(compiler->int32_type->llvm_type, int_expr->data, 0);
    return value;
}

static typed_value_t *compile_bool_expr(compiler_t *compiler, bool_expr_t *bool_expr) {
    typed_value_t *value = malloc_s(typed_value_t);
    value->type = compiler->bool_type;
    value->value = LLVMConstInt(compiler->bool_type->llvm_type, bool_expr->data, 0);
    return value;
}

static typed_value_t *compile_variable_expr(compiler_t *compiler, variable_expr_t *variable_expr) {
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
/*
 *
        L"==",
        L"!=",
        L"<",
        L"<=",
        L">",
        L">=",
        L"+",
        L"-",
        L"*",
        L"/",
 */

static typed_value_t *create_int_binop_inst(compiler_t *compiler, wchar_t *op, LLVMValueRef lhs, LLVMValueRef rhs) {
    typed_value_t *value = malloc_s(typed_value_t);

    if (!wcscmp(L"+", op)) {
        value->type = compiler->int32_type;
        value->value = LLVMBuildAdd(compiler->builder, lhs, rhs, "add_tmp");
        return value;
    }

    if (!wcscmp(L"-", op)) {
        value->type = compiler->int32_type;
        value->value = LLVMBuildSub(compiler->builder, lhs, rhs, "sub_tmp");
        return value;
    }

    if (!wcscmp(L"*", op)) {
        value->type = compiler->int32_type;
        value->value = LLVMBuildMul(compiler->builder, lhs, rhs, "mul_tmp");
        return value;
    }

    if (!wcscmp(L"/", op)) {
        value->type = compiler->int32_type;
        value->value = LLVMBuildSDiv(compiler->builder, lhs, rhs, "mul_tmp");
        return value;
    }

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
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntSLT, lhs, rhs, "slt_tmp");
        return value;
    }

    if (!wcscmp(L"<=", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntSLE, lhs, rhs, "sle_tmp");
        return value;
    }

    if (!wcscmp(L">", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntSGT, lhs, rhs, "sgt_tmp");
        return value;
    }

    if (!wcscmp(L">=", op)) {
        value->type = compiler->bool_type;
        value->value = LLVMBuildICmp(compiler->builder, LLVMIntSGE, lhs, rhs, "sge_tmp");
        return value;
    }

    free(value);
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

static typed_value_t *compile_binary_expr(compiler_t *compiler, binary_expr_t *binary_expr) {
    typed_value_t *lhs = compile_expr(compiler, binary_expr->data->lhs, 0);
    if (lhs == NULL) return NULL;

    typed_value_t *rhs = compile_expr(compiler, binary_expr->data->rhs, 0);
    if (rhs == NULL) {
        free(lhs);
        return NULL;
    }

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
    } else if (lhs->type == compiler->int32_type) {
        value = create_int_binop_inst(compiler, op, lhs->value, rhs->value);
    }

    free(lhs);
    free(rhs);

    if (value == NULL) {
        fprintf(stderr, "Unknown binary operator '%ls' for type %ls!\n", op, lhs->type->name);
        return NULL;
    }

    return value;
}

static typed_value_t *compile_call_expr(compiler_t *compiler, call_expr_t *call_expr) {
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

static typed_value_t *compile_if_expr(compiler_t *compiler, if_expr_t *if_expr, int is_stmt) {
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

static typed_value_t *compile_expr(compiler_t *compiler, expr_t *expr, int is_stmt) {
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
    }
}

static annotated_prototype_t *annotate_prototype(compiler_t *compiler, prototype_t *prototype) {
    annotated_prototype_t *annotated_prototype = malloc_s(annotated_prototype_t);
    annotated_prototype->name = prototype->name;
    annotated_prototype->is_extern = prototype->is_extern;
    annotated_prototype->arguments = ptr_list_new();

    size_t i;
    size_t arg_count = ptr_list_size(prototype->arguments);
    for (i = 0; i < arg_count; i++) {
        typed_ast_value_t *typed_arg = (typed_ast_value_t *) ptr_list_at(prototype->arguments, i);
        type_t *arg_type = find_type(compiler, typed_arg->type);

        if (arg_type == NULL) {
            fprintf(stderr, "Unknown type %ls\n", typed_arg->type);
            return NULL;
        }

        annotated_typed_arg_t *annotated_typed_arg = malloc_s(annotated_typed_arg_t);
        annotated_typed_arg->name = typed_arg->name;
        annotated_typed_arg->type = arg_type;

        ptr_list_push(annotated_prototype->arguments, annotated_typed_arg);
    }

    annotated_prototype->return_type = find_type(compiler, prototype->return_type);
    if (annotated_prototype->return_type == NULL) {
        fprintf(stderr, "Unknown type %ls\n", prototype->return_type);
        return NULL;
    }

    return annotated_prototype;
}

static LLVMTypeRef get_function_type(annotated_prototype_t *prototype) {
    size_t i;
    size_t arg_count = ptr_list_size(prototype->arguments);
    ptr_list_t *arg_types = ptr_list_new();
    for (i = 0; i < arg_count; i++) {
        annotated_typed_arg_t *arg = (type_t *) ptr_list_at(prototype->arguments, i);
        ptr_list_push(arg_types, arg->type->llvm_type);
    }

    LLVMTypeRef function_type = LLVMFunctionType(
            prototype->return_type->llvm_type,
            (LLVMTypeRef *) ptr_list_raw(arg_types),
            arg_count,
            0
    );

    ptr_list_free(arg_types);

    return function_type;
}

static function_t *compile_prototype(compiler_t *compiler, prototype_t *p) {
    annotated_prototype_t *prototype = annotate_prototype(compiler, p);
    LLVMTypeRef function_type = get_function_type(prototype);

    LLVMValueRef function = LLVMAddFunction(compiler->module, to_mbs(prototype->name), function_type);
    LLVMSetLinkage(function, LLVMExternalLinkage);

    function_t *function_obj = (function_t *) malloc(sizeof(function_t));
    function_obj->prototype = prototype;
    function_obj->type = function_type;
    function_obj->function = function;

    ptr_list_push(compiler->functions, function_obj);

    size_t i;
    for (i = 0; i < ptr_list_size(prototype->arguments); i++) {
        annotated_typed_arg_t *arg = (annotated_typed_arg_t *) ptr_list_at(prototype->arguments, i);

        LLVMValueRef param = LLVMGetParam(function, i);
        LLVMSetValueName2(param, to_mbs(arg->name), strlen(wcstombs_buffer));
    }

    return function_obj;
}

static typed_value_t *compile_return(compiler_t *compiler, expr_t *value_expr) {
    typed_value_t *value = compile_expr(compiler, value_expr, 0);
    if (value == NULL) return NULL;

    typed_value_t *return_value = malloc_s(typed_value_t);
    return_value->type = compiler->void_type;
    return_value->value = LLVMBuildRet(compiler->builder, value->value);
    return value;
}

static typed_value_t *compile_assignment(compiler_t *compiler, assignment_stmt_data_t *data) {
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

    variable->flags |= VAR_IS_INITIALIZED;

    typed_value_t *ret = malloc_s(typed_value_t);
    ret->type = compiler->void_type;
    ret->value = LLVMBuildStore(compiler->builder, value->value, variable->value);
    return ret;
}

static typed_value_t *compile_while(compiler_t *compiler, while_stmt_data_t *data) {
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

static typed_value_t *compile_stmt(compiler_t *compiler, stmt_t *stmt) {
    switch (stmt->stmt_type) {
        case STMT_RETURN:
            return compile_return(compiler, ((return_stmt_t *) stmt)->value);
        case STMT_EXPR:
            return compile_expr(compiler, ((expr_stmt_t *) stmt)->expr, 1);
        case STMT_ASSIGNMENT:
            return compile_assignment(compiler, ((assignment_stmt_t *) stmt)->data);
        case STMT_WHILE:
            return compile_while(compiler, ((while_stmt_t *) stmt)->data);
        case STMT_FUNCTION:
            wprintf(L"Functions are only allowed as top level statements!\n");
            return NULL;
        case STMT_EXTERN:
            wprintf(L"Extern declarations are only allowed as top level statements!\n");
            return NULL;
    }
}

static function_t *compile_function(compiler_t *compiler, function_stmt_t *function_stmt) {
    if (find_function_by_name(compiler, function_stmt->data->prototype->name) != NULL) {
        fprintf(stderr, "Redefinition of function %ls!\n", function_stmt->data->prototype->name);
        return NULL;
    }

    function_t *function_obj = compile_prototype(compiler, function_stmt->data->prototype);
    LLVMValueRef function = function_obj->function;

    LLVMBasicBlockRef bb = LLVMAppendBasicBlockInContext(compiler->context, function, "entry");
    LLVMPositionBuilderAtEnd(compiler->builder, bb);

    // Reset variable list

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->variables); i++) {
        free(ptr_list_at(compiler->variables, i));
    }

    ptr_list_free(compiler->variables);
    compiler->variables = ptr_list_new();

    // Add function parameters

    for (i = 0; i < LLVMCountParams(function); i++) {
        annotated_typed_arg_t *arg = ((annotated_typed_arg_t *) ptr_list_at(function_obj->prototype->arguments, i));

        variable_t *variable = malloc_s(variable_t);
        variable->name = arg->name;
        variable->value = LLVMGetParam(function, i);
        variable->type = arg->type;
        variable->flags = VAR_IS_PARAM;
        ptr_list_push(compiler->variables, variable);
    }

    // Add stack variables

    for (i = 0; i < ptr_list_size(function_stmt->data->variables); i++) {
        typed_ast_value_t *ast_var = (typed_ast_value_t *) ptr_list_at(function_stmt->data->variables, i);

        type_t *type = find_type(compiler, ast_var->type);
        if (type == NULL) {
            fprintf(stderr, "Unknown type %ls\n", ast_var->type);
            return NULL;
        }

        variable_t *var = malloc_s(variable_t);
        var->name = ast_var->name;
        var->value = LLVMBuildAlloca(compiler->builder, type->llvm_type, to_mbs(ast_var->name));
        var->type = type;
        var->flags = ast_var->flags;
        ptr_list_push(compiler->variables, var);
    }

    // Compile body

    int has_ret = 0;
    for (i = 0; i < ptr_list_size(function_stmt->data->body); i++) {
        stmt_t *stmt = (stmt_t *) ptr_list_at(function_stmt->data->body, i);
        typed_value_t *value = compile_stmt(compiler, stmt);
        if (value == NULL) {
            LLVMDeleteFunction(function);
            return NULL;
        }

        if (stmt->stmt_type == STMT_RETURN) {
            has_ret = 1;
            break;
        }
    }

    if (!has_ret) {
        if (function_obj->prototype->return_type != compiler->void_type) {
            fprintf(stderr, "Missing return in non-void function %ls!\n", function_obj->prototype->name);
            LLVMDeleteFunction(function);
            return NULL;
        }

        LLVMBuildRetVoid(compiler->builder);
    }

    if (LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
        LLVMDumpValue(function);
        LLVMDeleteFunction(function);
        return NULL;
    }

    if (compiler->opt_level == OPT_ALL) {
        LLVMInitializeFunctionPassManager(compiler->function_pm);
        LLVMRunFunctionPassManager(compiler->function_pm, function);
        LLVMFinalizeFunctionPassManager(compiler->function_pm);
    }

    return function_obj;
}

static function_t *compile_top_level_statement(compiler_t *compiler, stmt_t *stmt) {
    switch (stmt->stmt_type) {
        case STMT_FUNCTION:
            return compile_function(compiler, (function_stmt_t *) stmt);
        case STMT_EXTERN: {
            function_t *function_obj = compile_prototype(compiler, ((extern_stmt_t *) stmt)->prototype);
            return function_obj;
        }
        default:
            fprintf(stderr, "Only functions and extern declarations are allowed as top level statements!\n");
            return NULL;
    }
}

compiler_t *compiler_new(ptr_list_t *stmts, compiler_opt_level_t opt_level) {
    compiler_t *compiler = (compiler_t *) malloc(sizeof(compiler_t));

    compiler->context = LLVMContextCreate();
    compiler->module = LLVMModuleCreateWithNameInContext("my_module", compiler->context);
    compiler->builder = LLVMCreateBuilderInContext(compiler->context);

    compiler->variables = ptr_list_new();
    compiler->functions = ptr_list_new();
    compiler->top_level_statements = stmts;

    compiler->void_type = create_type(L"Void", LLVMVoidTypeInContext(compiler->context));
    compiler->bool_type = create_type(L"Bool", LLVMInt1TypeInContext(compiler->context));
    compiler->int32_type = create_type(L"Int32", LLVMInt32TypeInContext(compiler->context));

    compiler->types = ptr_list_new();
    ptr_list_push(compiler->types, compiler->void_type);
    ptr_list_push(compiler->types, compiler->bool_type);
    ptr_list_push(compiler->types, compiler->int32_type);

    compiler->opt_level = opt_level;
    compiler->function_pm = NULL;

    if (opt_level != OPT_NONE) {
        compiler->function_pm = LLVMCreateFunctionPassManagerForModule(compiler->module);

        LLVMAddInstructionCombiningPass(compiler->function_pm);
        LLVMAddReassociatePass(compiler->function_pm);
        LLVMAddGVNPass(compiler->function_pm);
        LLVMAddCFGSimplificationPass(compiler->function_pm);
        LLVMAddPromoteMemoryToRegisterPass(compiler->function_pm);
        LLVMAddInstructionCombiningPass(compiler->function_pm);
        LLVMAddReassociatePass(compiler->function_pm);
    }

    return compiler;
}

int compiler_compile(compiler_t *compiler) {
    size_t i;
    for (i = 0; i < ptr_list_size(compiler->top_level_statements); i++) {
        stmt_t *stmt = (stmt_t *) ptr_list_at(compiler->top_level_statements, i);

        if (compile_top_level_statement(compiler, stmt) == NULL) {
            return 1;
        }
    }

    return 0;
}

void compiler_dump_all(compiler_t *compiler, int open_cfg) {
    LLVMDumpModule(compiler->module);

    char *msg = NULL;
    LLVMVerifyModule(compiler->module, LLVMPrintMessageAction, &msg);
    if (msg != NULL) LLVMDisposeMessage(msg);

    if (!open_cfg) return;

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->functions); i++) {
        function_t *function = ptr_list_at(compiler->functions, i);
        if (function->prototype->is_extern) {
            fprintf(stderr, "Skipping extern function %ls for CFG visualization\n", function->prototype->name);
            continue;
        }

        LLVMViewFunctionCFG(function->function);
    }
}

int compiler_is_main_void(compiler_t *compiler) {
    return find_function_by_name(compiler, L"main")->prototype->return_type == compiler->void_type;
}

LLVMValueRef compiler_get_main(compiler_t *compiler) {
    return compiler_get_function(compiler, L"main");
}

LLVMModuleRef compiler_get_module(compiler_t *compiler) {
    return compiler->module;
}

LLVMValueRef compiler_get_function(compiler_t *compiler, wchar_t *name) {
    function_t *function = find_function_by_name(compiler, name);
    if (function == NULL) return NULL;

    return function->function;
}
