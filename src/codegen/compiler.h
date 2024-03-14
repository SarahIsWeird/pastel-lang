//
// Created by sarah on 3/12/24.
//

#ifndef PASTEL_COMPILER_H
#define PASTEL_COMPILER_H

#include <llvm-c/Types.h>
#include "../util/ptr_list.h"

typedef enum compiler_opt_level_t {
    OPT_NONE,
    OPT_ALL,
} compiler_opt_level_t;

typedef struct compiler_t compiler_t;

compiler_t *compiler_new(ptr_list_t *stmts, compiler_opt_level_t opt_level);
int compiler_compile(compiler_t *compiler);

void compiler_dump_all(compiler_t *compiler, int open_cfg);

int compiler_is_main_void(compiler_t *compiler);
LLVMValueRef compiler_get_main(compiler_t *compiler);
LLVMModuleRef compiler_get_module(compiler_t *compiler);
LLVMValueRef compiler_get_function(compiler_t *compiler, wchar_t *name);

#endif //PASTEL_COMPILER_H
