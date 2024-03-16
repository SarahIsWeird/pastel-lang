//
// Created by sarah on 3/14/24.
//

#ifndef PASTEL_TYPES_H
#define PASTEL_TYPES_H

#include "codegen/compiler.h"
#include "parser/ast.h"

#include <wchar.h>

#include <llvm-c/Types.h>

typedef enum type_flags_t {
    TYPE_ANY = 0,
    TYPE_INT = 0x1,
    TYPE_FLOAT = 0x2,
    TYPE_SIGNED = 0x4,
    TYPE_UNSIGNED = 0,
    TYPE_POINTER = 0x8,
} type_flags_t;

typedef struct type_t type_t;

typedef struct type_metadata_t {
    type_t *inner_type;
} type_metadata_t;

struct type_t {
    wchar_t *name;
    LLVMTypeRef llvm_type;
    type_flags_t flags;
    int size; // Size in bytes
    type_metadata_t *metadata;
};

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

    type_t *int8_type;
    type_t *int16_type;
    type_t *int32_type;
    type_t *int64_type;

    type_t *uint8_type;
    type_t *uint16_type;
    type_t *uint32_type;
    type_t *uint64_type;

    type_t *float32_type;
    type_t *float64_type;

    compiler_opt_level_t opt_level;
    LLVMPassManagerRef function_pm;
};

#endif //PASTEL_TYPES_H
