//
// Created by sarah on 3/14/24.
//

#ifndef PASTEL_CASTING_H
#define PASTEL_CASTING_H

#include "utils.h"

typed_value_t *cast_to_int(compiler_t *compiler, typed_value_t *value, type_t *dest_type);
typed_value_t *cast_value(compiler_t *compiler, typed_value_t *value, type_t *dest_type);
void do_type_coercion(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out);

#endif //PASTEL_CASTING_H
