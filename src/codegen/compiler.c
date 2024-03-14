//
// Created by sarah on 3/12/24.
//

#include "utils.h"
#include "parser/ast.h"
#include "stmt/stmt.h"

#include <stdlib.h>
#include <stdio.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/Utils.h>

static void init_types(compiler_t *compiler) {
    compiler->void_type = create_type(L"Void", LLVMVoidTypeInContext(compiler->context), TYPE_ANY, 0);
    compiler->bool_type = create_type(L"Bool", LLVMInt1TypeInContext(compiler->context), TYPE_ANY, 1);

    compiler->int8_type = create_type(L"Int8", LLVMInt8TypeInContext(compiler->context), TYPE_INT | TYPE_SIGNED, 1);
    compiler->int16_type = create_type(L"Int16", LLVMInt16TypeInContext(compiler->context), TYPE_INT | TYPE_SIGNED, 2);
    compiler->int32_type = create_type(L"Int32", LLVMInt32TypeInContext(compiler->context), TYPE_INT | TYPE_SIGNED, 4);
    compiler->int64_type = create_type(L"Int64", LLVMInt64TypeInContext(compiler->context), TYPE_INT | TYPE_SIGNED, 8);

    compiler->uint8_type = create_type(L"UInt8", LLVMInt8TypeInContext(compiler->context), TYPE_INT | TYPE_UNSIGNED, 1);
    compiler->uint16_type = create_type(L"UInt16", LLVMInt16TypeInContext(compiler->context), TYPE_INT | TYPE_UNSIGNED, 2);
    compiler->uint32_type = create_type(L"UInt32", LLVMInt32TypeInContext(compiler->context), TYPE_INT | TYPE_UNSIGNED, 4);
    compiler->uint64_type = create_type(L"UInt64", LLVMInt64TypeInContext(compiler->context), TYPE_INT | TYPE_UNSIGNED, 8);

    compiler->types = ptr_list_new();
    ptr_list_push(compiler->types, compiler->void_type);
    ptr_list_push(compiler->types, compiler->bool_type);

    ptr_list_push(compiler->types, compiler->int8_type);
    ptr_list_push(compiler->types, compiler->int16_type);
    ptr_list_push(compiler->types, compiler->int32_type);
    ptr_list_push(compiler->types, compiler->int64_type);

    ptr_list_push(compiler->types, compiler->uint8_type);
    ptr_list_push(compiler->types, compiler->uint16_type);
    ptr_list_push(compiler->types, compiler->uint32_type);
    ptr_list_push(compiler->types, compiler->uint64_type);
}

compiler_t *compiler_new(ptr_list_t *stmts, compiler_opt_level_t opt_level) {
    compiler_t *compiler = (compiler_t *) malloc(sizeof(compiler_t));

    compiler->context = LLVMContextCreate();
    compiler->module = LLVMModuleCreateWithNameInContext("my_module", compiler->context);
    compiler->builder = LLVMCreateBuilderInContext(compiler->context);

    compiler->variables = ptr_list_new();
    compiler->functions = ptr_list_new();
    compiler->top_level_statements = stmts;

    init_types(compiler);

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
