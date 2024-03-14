//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_IF_H
#define PASTEL_IF_H

#include "../types.h"

typed_value_t *compile_if_expr(compiler_t *compiler, if_expr_t *if_expr, int is_stmt);

#endif //PASTEL_IF_H
