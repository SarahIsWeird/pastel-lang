//
// Created by sarah on 3/12/24.
//

#include "parser/ast.h"

#include <wchar.h>

void print_indent(int indent) {
    int i;
    for (i = 0; i < indent; i++) {
        wprintf(L"%lc", L' ');
    }
}

void print_prototype(prototype_t *prototype, int indent) {
    print_indent(indent);
    wprintf(L"Name: %ls\n", prototype->name);
    print_indent(indent);
    wprintf(L"Return type: %ls\n", prototype->return_type);
    print_indent(indent);
    wprintf(L"Is extern: %ls\n", prototype->is_extern ? L"yes" : L"no");
    print_indent(indent);
    wprintf(L"Arguments: (%lu)\n", ptr_list_size(prototype->arguments));

    size_t i;
    for (i = 0; i < ptr_list_size(prototype->arguments); i++) {
        typed_ast_value_t *arg = (typed_ast_value_t *) ptr_list_get(prototype->arguments);
        print_indent(indent + 2);
        wprintf(L"%ls (%ls)\n", arg->name, arg->type);
    }
}

void print_stmt(stmt_t *stmt, int indent) {
    size_t i;
    function_stmt_data_t *func_data;
    assignment_stmt_data_t *ass_data; // hehe
    while_stmt_data_t *while_data;

    print_indent(indent);

    switch (stmt->stmt_type) {
        case STMT_RETURN:
            wprintf(L"Return\n");
            print_expr(((return_stmt_t *) stmt)->value, indent + 2);
            break;
        case STMT_EXPR:
            wprintf(L"Expr\n");
            print_expr(((expr_stmt_t *) stmt)->expr, indent + 2);
            break;
        case STMT_FUNCTION:
            func_data = ((function_stmt_t *) stmt)->data;
            wprintf(L"Function\n");

            print_indent(indent + 2);
            wprintf(L"Prototype:\n");
            print_prototype(func_data->prototype, indent + 4);

            print_indent(indent + 2);
            wprintf(L"Variables: (%lu)\n", ptr_list_size(func_data->variables));
            for (i = 0; i < ptr_list_size(func_data->variables); i++) {
                typed_ast_value_t *var = (typed_ast_value_t *) ptr_list_at(func_data->variables, i);
                print_indent(indent + 4);
                wprintf(L"%ls (%ls)\n", var->name, var->type);
            }

            print_indent(indent + 2);
            wprintf(L"Body:\n");
            for (i = 0; i < ptr_list_size(func_data->body); i++) {
                stmt_t *body_stmt = (stmt_t *) ptr_list_at(func_data->body, i);
                print_stmt(body_stmt, indent + 4);
            }

            break;
        case STMT_EXTERN:
            wprintf(L"Extern\n");
            print_prototype(((extern_stmt_t *) stmt)->prototype, indent + 2);
            break;
        case STMT_ASSIGNMENT:
            ass_data = ((assignment_stmt_t *) stmt)->data;
            wprintf(L"Assignment to %ls\n", ass_data->name);
            print_expr(ass_data->value, indent + 2);
            break;
        case STMT_WHILE:
            while_data = ((while_stmt_t *) stmt)->data;
            wprintf(L"While\n");

            print_indent(indent + 2);
            wprintf(L"Condition:\n");
            print_expr(while_data->condition, indent + 4);

            print_indent(indent + 2);
            wprintf(L"Body:\n");
            for (i = 0; i < ptr_list_size(while_data->body); i++) {
                stmt_t *body_stmt = (stmt_t *) ptr_list_at(while_data->body, i);
                print_stmt(body_stmt, indent + 4);
            }
            break;
    }
}

void print_expr(expr_t *expr, int indent) {
    size_t i;
    unary_expr_data_t *unary_expr_data;
    binary_expr_data_t *binary_expr_data;
    call_expr_data_t *call_expr_data;
    if_expr_data_t *if_expr_data;
    cast_expr_data_t *cast_expr_data;

    print_indent(indent);

    switch (expr->expr_type) {
        case EXPR_BOOL:
            wprintf(L"Bool: %ls\n", ((bool_expr_t *) expr)->data ? L"true" : L"false");
            break;
        case EXPR_INT:
            wprintf(L"Int: %d\n", ((int_expr_t *) expr)->data);
            break;
        case EXPR_FLOAT:
            wprintf(L"Float: %d\n", *((float_expr_t *) expr)->data);
            break;
        case EXPR_VARIABLE:
            wprintf(L"Variable: %ls\n", ((variable_expr_t *) expr)->name);
            break;
        case EXPR_UNARY:
            unary_expr_data = ((unary_expr_t *) expr)->data;
            wprintf(L"Unary expression: %ls\n", unary_expr_data->op);
            print_expr(unary_expr_data->value, indent + 2);
            break;
        case EXPR_BINARY:
            binary_expr_data = ((binary_expr_t *) expr)->data;
            wprintf(L"Binary expression: %ls\n", binary_expr_data->op);
            print_expr(binary_expr_data->lhs, indent + 2);
            print_expr(binary_expr_data->rhs, indent + 2);
            break;
        case EXPR_CALL:
            call_expr_data = ((call_expr_t *) expr)->data;
            wprintf(L"Call: %ls\n", call_expr_data->callee_name);
            for (i = 0; i < ptr_list_size(call_expr_data->arguments); i++) {
                expr_t *arg = (expr_t *) ptr_list_at(call_expr_data->arguments, i);
                print_expr(arg, indent + 2);
            }
            break;
        case EXPR_IF:
            if_expr_data = ((if_expr_t *) expr)->data;
            wprintf(L"If\n");

            print_indent(indent + 2);
            wprintf(L"Condition:\n");;
            print_expr(if_expr_data->condition, indent + 4);

            print_indent(indent + 2);
            wprintf(L"Then: (Statements: %lu)\n", ptr_list_size(if_expr_data->then_stmts));
            for (i = 0; i < ptr_list_size(if_expr_data->then_stmts); i++) {
                stmt_t *stmt= (stmt_t *) ptr_list_at(if_expr_data->then_stmts, i);
                print_stmt(stmt, indent + 4);
            }

            if (if_expr_data->else_stmts != NULL) {
                print_indent(indent + 2);
                wprintf(L"Else: (Statements: %lu)\n", ptr_list_size(if_expr_data->else_stmts));
                for (i = 0; i < ptr_list_size(if_expr_data->else_stmts); i++) {
                    stmt_t *stmt = (stmt_t *) ptr_list_at(if_expr_data->else_stmts, i);
                    print_stmt(stmt, indent + 4);
                }
            }

            break;
        case EXPR_CAST:
            cast_expr_data = ((cast_expr_t *) expr)->data;
            wprintf(L"Cast to %ls\n", cast_expr_data->type);
            print_expr(cast_expr_data->value, indent + 2);
            break;
    }
}
