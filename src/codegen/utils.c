//
// Created by sarah on 3/14/24.
//

#include "utils.h"

#include <stdlib.h>
#include <stdio.h>

#include <llvm-c/Core.h>

static char wcstombs_buffer[512];
char *to_mbs(const wchar_t *str) {
    if (wcstombs(wcstombs_buffer, str, 512) == 512) {
        wcstombs_buffer[511] = 0;
    }

    return wcstombs_buffer;
}

type_t *create_type(wchar_t *name, LLVMTypeRef llvm_type, type_flags_t flags, int size) {
    type_t *type = malloc_s(type_t);
    type->name = name;
    type->llvm_type = llvm_type;
    type->flags = flags;
    type->size = size;
    return type;
}

type_t *find_type(compiler_t *compiler, const wchar_t *name) {
    if (name == NULL) return compiler->void_type;

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->types); i++) {
        type_t *type = (type_t *) ptr_list_at(compiler->types, i);
        if (!wcscmp(name, type->name)) return type;
    }

    return NULL;
}

variable_t *find_variable(compiler_t *compiler, const wchar_t *name) {
    if (name == NULL) return NULL;

    size_t i;
    for (i = 0; i < ptr_list_size(compiler->variables); i++) {
        variable_t *variable = (variable_t *) ptr_list_at(compiler->variables, i);
        if (!wcscmp(name, variable->name)) return variable;
    }

    return NULL;
}

function_t *find_function_by_name(compiler_t *compiler, const wchar_t *name) {
    size_t i;
    for (i = 0; i < ptr_list_size(compiler->functions); i++) {
        function_t *function = (function_t *) ptr_list_at(compiler->functions, i);
        if (!wcscmp(name, function->prototype->name)) return function;
    }

    return NULL;
}

annotated_prototype_t *annotate_prototype(compiler_t *compiler, prototype_t *prototype) {
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

LLVMTypeRef get_function_type(annotated_prototype_t *prototype) {
    size_t i;
    size_t arg_count = ptr_list_size(prototype->arguments);
    ptr_list_t *arg_types = ptr_list_new();
    for (i = 0; i < arg_count; i++) {
        annotated_typed_arg_t *arg = (annotated_typed_arg_t *) ptr_list_at(prototype->arguments, i);
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
