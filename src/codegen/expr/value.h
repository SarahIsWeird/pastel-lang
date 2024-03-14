//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_VALUE_H
#define PASTEL_VALUE_H

#include "../types.h"

typed_value_t *compile_int_expr(compiler_t *compiler, int_expr_t *int_expr);
typed_value_t *compile_bool_expr(compiler_t *compiler, bool_expr_t *bool_expr);
typed_value_t *compile_variable_expr(compiler_t *compiler, variable_expr_t *variable_expr);
typed_value_t *compile_cast_expr(compiler_t *compiler, cast_expr_t *expr);

#endif //PASTEL_VALUE_H
