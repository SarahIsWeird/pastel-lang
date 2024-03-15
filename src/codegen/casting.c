//
// Created by sarah on 3/14/24.
//

#include "casting.h"

#include <stdio.h>

#include <llvm-c/Core.h>

#define is_number(v) ((v->type->flags & (TYPE_INT | TYPE_FLOAT)) != 0)
#define cant_cast() fprintf(stderr, "Can't cast from %ls to %ls!\n", value->type->name, dest_type->name); return NULL

typedef typed_value_t *(*cast_function_t)(compiler_t *, typed_value_t *, type_t *);

typed_value_t *cast_to_int(compiler_t *compiler, typed_value_t *value, type_t *dest_type) {
    if (!is_number(value)) {
        cant_cast();
    }

    typed_value_t *cast_value = malloc_s(typed_value_t);
    cast_value->type = dest_type;

    int is_signed = (dest_type->flags & TYPE_SIGNED) != 0;

    if ((value->type->flags & TYPE_FLOAT) != 0) {
        if (is_signed) {
            cast_value->value = LLVMBuildFPToSI(compiler->builder, value->value, dest_type->llvm_type, "cast_tmp");;
        } else {
            cast_value->value = LLVMBuildFPToUI(compiler->builder, value->value, dest_type->llvm_type, "cast_tmp");;
        }

        return cast_value;
    }

    if (dest_type->size > value->type->size) {
        // The new type can fit all possible values of the old one, so we don't do signed conversion.
        is_signed = 0;
    }

    cast_value->value = LLVMBuildIntCast2(compiler->builder, value->value, dest_type->llvm_type, is_signed, "cast_tmp");
    return cast_value;
}

typed_value_t *cast_to_float(compiler_t *compiler, typed_value_t *value, type_t *dest_type) {
    if (!is_number(value)) {
        cant_cast();
    }

    typed_value_t *cast_value = malloc_s(typed_value_t);
    cast_value->type = dest_type;

    type_flags_t src_flags = value->type->flags;
    if ((src_flags & TYPE_INT) != 0) {
        if ((src_flags & TYPE_SIGNED) != 0) {
            cast_value->value = LLVMBuildSIToFP(compiler->builder, value->value, dest_type->llvm_type, "cast_tmp");
        } else {
            cast_value->value = LLVMBuildUIToFP(compiler->builder, value->value, dest_type->llvm_type, "cast_tmp");
        }

        return cast_value;
    }

    cast_value->value = LLVMBuildFPCast(compiler->builder, value->value, dest_type->llvm_type, "cast_tmp");
    return cast_value;
}

typed_value_t *cast_value(compiler_t *compiler, typed_value_t *value, type_t *dest_type) {
    if ((dest_type->flags & TYPE_INT) != 0) {
        return cast_to_int(compiler, value, dest_type);
    }

    if ((dest_type->flags & TYPE_FLOAT) != 0) {
        return cast_to_float(compiler, value, dest_type);
    }

    cant_cast();
}

#define is_int(tv) (((tv)->type->flags & TYPE_INT) != 0)
#define is_unsigned(tv) (((tv)->type->flags & TYPE_UNSIGNED) != 0)
#define is_float(tv) (((tv)->type->flags & TYPE_FLOAT) != 0)

static void coerce_with(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out, cast_function_t cast_function) {
    typed_value_t *lhs = *lhs_out;
    typed_value_t *rhs = *rhs_out;

    int is_rhs_smaller = rhs->type->size < lhs->type->size;
    typed_value_t *value_to_coerce = is_rhs_smaller ? rhs : lhs;
    type_t *type_to_coerce_to = is_rhs_smaller ? lhs->type : rhs->type;

    typed_value_t *coerced_value = (*cast_function)(compiler, value_to_coerce, type_to_coerce_to);

    if (is_rhs_smaller) {
        rhs = coerced_value;
    } else {
        lhs = coerced_value;
    }

    *lhs_out = lhs;
    *rhs_out = rhs;
}

static void do_int_type_coercion(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out) {    // We don't do type coercion when signedness isn't the same.
    if (is_unsigned(*lhs_out) != is_unsigned(*rhs_out)) return;

    coerce_with(compiler, lhs_out, rhs_out, cast_to_int);
}

static void do_float_type_coercion(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out) {
    coerce_with(compiler, lhs_out, rhs_out, cast_to_float);
}

void do_type_coercion(compiler_t *compiler, typed_value_t **lhs_out, typed_value_t **rhs_out) {
    typed_value_t *lhs = *lhs_out;
    typed_value_t *rhs = *rhs_out;

    if (lhs->type == rhs->type) return;

    if (is_int(lhs) && is_int(rhs)) {
        do_int_type_coercion(compiler, lhs_out, rhs_out);
        return;
    }

    if (is_float(lhs) && is_float(rhs)) {
        do_float_type_coercion(compiler, lhs_out, rhs_out);
        return;
    }
}
