//
// Created by sarah on 3/15/24.
//

#ifndef PASTEL_VALUE_H
#define PASTEL_VALUE_H

#include "../types.h"

typed_value_t *compile_return(compiler_t *compiler, expr_t *value_expr);
typed_value_t *compile_assignment(compiler_t *compiler, assignment_stmt_data_t *data);

#endif //PASTEL_VALUE_H
