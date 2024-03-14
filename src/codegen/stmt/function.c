//
// Created by sarah on 3/15/24.
//

#include "function.h"

#include "stmt.h"
#include "../utils.h"

#include <stdio.h>
#include <string.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

function_t *compile_prototype(compiler_t *compiler, prototype_t *p) {
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
        char *mbs_str = to_mbs(arg->name);
        LLVMSetValueName2(param, mbs_str, strlen(mbs_str));
    }

    return function_obj;
}

function_t *compile_function(compiler_t *compiler, function_stmt_t *function_stmt) {
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
