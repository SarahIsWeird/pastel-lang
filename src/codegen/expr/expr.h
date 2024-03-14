//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_EXPR_H
#define PASTEL_EXPR_H

#include "../types.h"

typed_value_t *compile_expr(compiler_t *compiler, expr_t *expr, int is_stmt);

#endif //PASTEL_EXPR_H
