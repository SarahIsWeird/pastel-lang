//
// Created by sarah on 3/14/24.
//

#ifndef PASTEL_BINOP_H
#define PASTEL_BINOP_H

#include "../utils.h"

typed_value_t *compile_unary_expr(compiler_t *compiler, unary_expr_t *binary_expr);
typed_value_t *compile_binary_expr(compiler_t *compiler, binary_expr_t *binary_expr);

#endif //PASTEL_BINOP_H
