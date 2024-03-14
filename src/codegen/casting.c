//
// Created by sarah on 3/14/24.
//

#include "casting.h"

#include <stdio.h>

#include <llvm-c/Core.h>

#define cant_cast() fprintf(stderr, "Can't cast from %ls to %ls!\n", value->type->name, dest_type->name); return NULL

typed_value_t *cast_to_int(compiler_t *compiler, typed_value_t *value, type_t *dest_type) {
    if ((value->type->flags & TYPE_INT) == 0) {
        cant_cast();
    }

    int is_signed = (dest_type->flags & TYPE_SIGNED) != 0;

    if (dest_type->size > value->type->size) {
        // The new type can fit all possible values of the old one, so we don't do signed conversion.
        is_signed = 0;
    }

    typed_value_t *cast_value = malloc_s(typed_value_t);
    cast_value->type = dest_type;
    cast_value->value = LLVMBuildIntCast2(compiler->builder, value->value, dest_type->llvm_type, is_signed, "cast_tmp");
    return cast_value;
}

typed_value_t *cast_value(compiler_t *compiler, typed_value_t *value, type_t *dest_type) {
    if ((dest_type->flags & TYPE_INT) != 0) {
        return cast_to_int(compiler, value, dest_type);
    }

    cant_cast();
}

#define is_int(tv) ((tv->type->flags & TYPE_INT) != 0)
#define is_unsigned(tv) ((tv->type->flags & TYPE_UNSIGNED) != 0)

void do_type_coercion(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out) {
    typed_value_t *lhs = *lhs_out;
    typed_value_t *rhs = *rhs_out;

    if (lhs->type == rhs->type) return;

    if (!is_int(lhs) || !is_int(rhs)) return;

    // We don't do type coercion when signedness isn't the same.
    if (is_unsigned(lhs) != is_unsigned(rhs)) return;

    int is_rhs_smaller = rhs->type->size < lhs->type->size;
    typed_value_t *value_to_coerce = is_rhs_smaller ? rhs : lhs;
    type_t *type_to_coerce_to = is_rhs_smaller ? lhs->type : rhs->type;

    typed_value_t *coerced_value = cast_to_int(compiler, value_to_coerce, type_to_coerce_to);

    if (is_rhs_smaller) {
        rhs = coerced_value;
    } else {
        lhs = coerced_value;
    }

    *lhs_out = lhs;
    *rhs_out = rhs;
}
