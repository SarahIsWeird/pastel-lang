//
// Created by sarah on 3/15/24.
//

#include "stmt.h"

#include "function.h"
#include "loop.h"
#include "value.h"
#include "../expr/expr.h"

#include <stdio.h>

typed_value_t *compile_stmt(compiler_t *compiler, stmt_t *stmt) {
    switch (stmt->stmt_type) {
        case STMT_RETURN:
            return compile_return(compiler, ((return_stmt_t *) stmt)->value);
        case STMT_EXPR:
            return compile_expr(compiler, ((expr_stmt_t *) stmt)->expr, 1);
        case STMT_ASSIGNMENT:
            return compile_assignment(compiler, ((assignment_stmt_t *) stmt)->data);
        case STMT_WHILE:
            return compile_while(compiler, ((while_stmt_t *) stmt)->data);
        case STMT_FUNCTION:
            wprintf(L"Functions are only allowed as top level statements!\n");
            return NULL;
        case STMT_EXTERN:
            wprintf(L"Extern declarations are only allowed as top level statements!\n");
            return NULL;
    }
}

function_t *compile_top_level_statement(compiler_t *compiler, stmt_t *stmt) {
    switch (stmt->stmt_type) {
        case STMT_FUNCTION:
            return compile_function(compiler, (function_stmt_t *) stmt);
        case STMT_EXTERN: {
            function_t *function_obj = compile_prototype(compiler, ((extern_stmt_t *) stmt)->prototype);
            return function_obj;
        }
        default:
            fprintf(stderr, "Only functions and extern declarations are allowed as top level statements!\n");
            return NULL;
    }
}
