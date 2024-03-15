//
// Created by sarah on 3/12/24.
//

#ifndef PASTEL_AST_H
#define PASTEL_AST_H

#include <stddef.h>
#include "../../src/util/ptr_list.h"

typedef enum expr_type_t {
    EXPR_BOOL,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_CALL,
    EXPR_IF,
    EXPR_CAST,
} expr_type_t;

typedef enum stmt_type_t {
    STMT_RETURN,
    STMT_EXPR,
    STMT_FUNCTION,
    STMT_EXTERN,
    STMT_ASSIGNMENT,
    STMT_WHILE,
} stmt_type_t;

typedef enum variable_flags_t {
    VAR_IS_PARAM = 1,
    VAR_IS_MUTABLE = 2,
    VAR_IS_IMMUTABLE = 0,
    VAR_IS_INITIALIZED = 4,
} variable_flags_t;

typedef struct typed_ast_value_t {
    wchar_t *name;
    wchar_t *type;
    variable_flags_t flags;
} typed_ast_value_t;

typedef struct prototype_t {
    wchar_t *name;
    wchar_t *return_type;
    int is_extern;
    ptr_list_t *arguments; // List<typed_ast_value_t *>
} prototype_t;

/* Expression data structs */

typedef struct expr_t expr_t;

typedef struct binary_expr_data_t {
    wchar_t *op;
    expr_t *lhs;
    expr_t *rhs;
} binary_expr_data_t;

typedef struct call_expr_data_t {
    wchar_t *callee_name;
    ptr_list_t *arguments; // List<expr_t *>
} call_expr_data_t;

typedef struct if_expr_data_t {
    expr_t *condition;
    ptr_list_t *then_stmts; // List<stmt_t *>
    ptr_list_t *else_stmts; // List<stmt_t *>
} if_expr_data_t;

typedef struct cast_expr_data_t {
    expr_t *value;
    wchar_t *type;
} cast_expr_data_t;

/* Expressions */

struct expr_t {
    expr_type_t expr_type;
    void *data;
};

typedef struct int_expr_t {
    expr_type_t expr_type;
    int data;
} int_expr_t;

typedef struct bool_expr_t {
    expr_type_t expr_type;
    int data;
} bool_expr_t;

typedef struct float_expr_t {
    expr_type_t expr_type;
    double *data;
} float_expr_t;

typedef struct variable_expr_t {
    expr_type_t expr_type;
    wchar_t *name;
} variable_expr_t;

typedef struct binary_expr_t {
    expr_type_t expr_type;
    binary_expr_data_t *data;
} binary_expr_t;

typedef struct call_expr_t {
    expr_type_t expr_type;
    call_expr_data_t *data;
} call_expr_t;

typedef struct if_expr_t {
    expr_type_t expr_type;
    if_expr_data_t *data;
} if_expr_t;

typedef struct cast_expr_t {
    expr_type_t expr_type;
    cast_expr_data_t *data;
} cast_expr_t;

/* Statement data */

typedef struct function_stmt_data_t {
    prototype_t *prototype;
    ptr_list_t *variables; // List<typed_ast_value_t *>
    ptr_list_t *body; // List<stmt_t *>
} function_stmt_data_t;

typedef struct assignment_stmt_data_t {
    wchar_t *name;
    expr_t *value;
} assignment_stmt_data_t;

typedef struct while_stmt_data_t {
    expr_t *condition;
    ptr_list_t *body; // List<stmt_t *>
} while_stmt_data_t;

/* Statements */

typedef struct stmt_t {
    stmt_type_t stmt_type;
    void *data;
} stmt_t;

typedef struct return_stmt_t {
    stmt_type_t stmt_type;
    expr_t *value;
} return_stmt_t;

typedef struct expr_stmt_t {
    stmt_type_t stmt_type;
    expr_t *expr;
} expr_stmt_t;

typedef struct function_stmt_t {
    stmt_type_t stmt_type;
    function_stmt_data_t *data;
} function_stmt_t;

typedef struct extern_stmt_t {
    stmt_type_t stmt_type;
    prototype_t *prototype;
} extern_stmt_t;

typedef struct assignment_stmt_t {
    stmt_type_t stmt_type;
    assignment_stmt_data_t *data;
} assignment_stmt_t;

typedef struct while_stmt_t {
    stmt_type_t stmt_type;
    while_stmt_data_t *data;
} while_stmt_t;

void print_stmt(stmt_t *stmt, int indent);
void print_expr(expr_t *expr, int indent);

#endif //PASTEL_AST_H
