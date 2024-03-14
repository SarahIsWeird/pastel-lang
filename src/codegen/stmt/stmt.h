//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_STMT_H
#define PASTEL_STMT_H

#include "../types.h"

typed_value_t *compile_stmt(compiler_t *compiler, stmt_t *stmt);
function_t *compile_top_level_statement(compiler_t *compiler, stmt_t *stmt);

#endif //PASTEL_STMT_H
