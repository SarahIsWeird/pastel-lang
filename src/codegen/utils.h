//
// Created by sarah on 3/14/24.
//

#ifndef PASTEL_UTILS_H
#define PASTEL_UTILS_H

#include "types.h"
#include "../util/util.h"

#include <stdlib.h>
#include <wchar.h>

char *to_mbs(const wchar_t *str);

type_t *create_type(wchar_t *name, LLVMTypeRef llvm_type, type_flags_t flags, int size);
type_t *find_type(compiler_t *compiler, const wchar_t *name);
variable_t *find_variable(compiler_t *compiler, const wchar_t *name);
function_t *find_function_by_name(compiler_t *compiler, const wchar_t *name);

annotated_prototype_t *annotate_prototype(compiler_t *compiler, prototype_t *prototype);
LLVMTypeRef get_function_type(annotated_prototype_t *prototype);

#endif //PASTEL_UTILS_H
