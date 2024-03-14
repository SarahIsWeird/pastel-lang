//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_FUNCTION_H
#define PASTEL_FUNCTION_H

#include "../types.h"

function_t *compile_prototype(compiler_t *compiler, prototype_t *p);
function_t *compile_function(compiler_t *compiler, function_stmt_t *function_stmt);

#endif //PASTEL_FUNCTION_H
