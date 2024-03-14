//
// Created by sarah on 3/12/24.
//

#include "parser/parser.h"
#include "parser/ast.h"
#include "lexer/token.h"

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

#define current_token ((token_t *) ptr_list_at(parser->tokens, parser->pos))
#define advance() do { parser->pos++; } while (0)

#define expected(str) fprintf(stderr, "[%lu:%lu] Expected %ls\n", current_token->token_pos.line, current_token->token_pos.column, str)
#define assert_token_type(tt, name) do if (current_token->type != tt) { expected(name); return NULL; } while (0)
#define assert_is_identifier() assert_token_type(TOKEN_IDENTIFIER, L"identifier")

#define get_identifier() (((token_identifier_t *) current_token)->value)
#define get_integer() (((token_integer_t *) current_token)->value)
#define get_operator() (((token_operator_t *) current_token)->op)

static expr_t *parse_expr(parser_t *parser);
static stmt_t *parse_stmt(parser_t *parser);

struct parser_t {
    ptr_list_t *tokens;
    size_t pos;
    function_stmt_t *current_function;
};

typedef struct operator_precedence_t {
    const wchar_t *op;
    int precedence;
} operator_precedence_t;

static operator_precedence_t operator_precedences[] = {
        { L"=", 1 },
        { L"==", 10 },
        { L"!=", 10 },
        { L"<=", 10 },
        { L">=", 10 },
        { L"<", 20 },
        { L">", 20 },
        { L"+", 30 },
        { L"-", 30 },
        { L"*", 40 },
        { L"/", 40 },
        { L"to", 100 }, // Cast binds very strongly
};

static size_t operator_count = sizeof(operator_precedences) / sizeof(operator_precedence_t);

parser_t *parser_new(ptr_list_t *tokens) {
    parser_t *parser = (parser_t *) malloc(sizeof(parser_t));

    parser->tokens = tokens;
    parser->pos = 0;
    parser->current_function = NULL;

    return parser;
}

void parser_free(parser_t *parser) {
    free(parser);
}

static int is_keyword(token_t *token, keyword_t keyword) {
    if (token->type != TOKEN_KEYWORD) return 0;
    return ((token_keyword_t *) token)->keyword == keyword;
}

static int is_char(token_t *token, wchar_t c) {
    if (token->type != TOKEN_CHAR) return 0;
    if (((token_char_t *) token)->value != c) return 0;

    return 1;
}

static int is_operator(token_t *token, wchar_t *op) {
    if (token->type != TOKEN_OPERATOR) return 0;
    if (wcscmp(((token_operator_t *) token)->op, op) != 0) return 0;

    return 1;
}

static void skip_end_of_statements(parser_t *parser) {
    while (current_token) {
        if (current_token->type == TOKEN_END_OF_STATEMENT) {
            advance();
            continue;
        }

        break;
    }
}

static int has_arg_separator(parser_t *parser, ptr_list_t *arguments) {
    if (ptr_list_size(arguments) != 0) {
        if (!is_char(current_token, L',')) {
            fprintf(stderr, "Expected ',' to separate function arguments\n");;
            return 0;
        }

        advance();
    }

    return 1;
}

static prototype_t *parse_prototype(parser_t *parser, int is_extern) {
    assert_is_identifier();
    wchar_t *name = get_identifier();
    advance();

    if (!is_char(current_token, L'(')) {
        fprintf(stderr, "Expected '(' after function name\n");
        return NULL;
    }
    advance();

    ptr_list_t *arguments = ptr_list_new();
    while (!is_char(current_token, L')')) {
        if (!has_arg_separator(parser, arguments)) return NULL;

        assert_is_identifier();
        wchar_t *arg_name = get_identifier();

        advance();
        if (!is_char(current_token, L':')) {
            expected(L"Expected ':' after function argument");
            return NULL;
        }

        advance();
        assert_is_identifier();
        wchar_t *arg_type = get_identifier();

        typed_ast_value_t *typed_arg = (typed_ast_value_t *) malloc(sizeof(typed_ast_value_t));
        typed_arg->name = arg_name;
        typed_arg->type = arg_type;
        typed_arg->flags = VAR_IS_PARAM | VAR_IS_IMMUTABLE;

        ptr_list_push(arguments, typed_arg);
        advance();
    }

    advance();
    wchar_t *return_type = NULL;
    if (is_char(current_token, L':')) {
        advance();
        assert_is_identifier();
        return_type = get_identifier();
        advance();
    }

    prototype_t *prototype = (prototype_t *) malloc(sizeof(prototype_t));
    prototype->name = name;
    prototype->return_type = return_type;
    prototype->arguments = arguments;
    prototype->is_extern = is_extern;
    return prototype;
}

static expr_t *parse_identifier(parser_t *parser) {
    wchar_t *identifier = get_identifier();
    advance();

    if (!is_char(current_token, L'(')) {
        variable_expr_t *expr = (variable_expr_t *) malloc(sizeof(variable_expr_t));
        expr->expr_type = EXPR_VARIABLE;
        expr->name = identifier;
        return (expr_t *) expr;
    }

    advance();

    ptr_list_t *call_args = ptr_list_new();
    while (!is_char(current_token, L')')) {
        if (!has_arg_separator(parser, call_args)) return NULL;

        expr_t *arg = parse_expr(parser);
        if (arg == NULL) return NULL;
        ptr_list_push(call_args, arg);
    }

    advance();

    call_expr_t *expr = (call_expr_t *) malloc(sizeof(call_expr_t));
    expr->expr_type = EXPR_CALL;
    expr->data = (call_expr_data_t *) malloc(sizeof(call_expr_data_t));
    expr->data->callee_name = identifier;
    expr->data->arguments = call_args;

    return (expr_t *) expr;
}

static expr_t *parse_int(parser_t *parser) {
    int_expr_t *expr = (int_expr_t *) malloc(sizeof(int_expr_t));
    expr->expr_type = EXPR_INT;
    expr->data = get_integer();
    advance();

    return (expr_t *) expr;
}

static ptr_list_t *parse_body(parser_t *parser);

static expr_t *parse_if(parser_t *parser) {
    advance();

    if_expr_data_t *data = (if_expr_data_t *) malloc(sizeof(if_expr_data_t));

    data->condition = parse_expr(parser);
    if (data->condition == NULL) return NULL;

    skip_end_of_statements(parser);

    data->then_stmts = parse_body(parser);
    if (data->then_stmts == NULL) return NULL;

    data->else_stmts = NULL;

    if (is_keyword(current_token, KEYWORD_ELSE)) {
        advance();
        skip_end_of_statements(parser);
        data->else_stmts = parse_body(parser);

        if (data->else_stmts == NULL) return NULL;
    }

    if_expr_t *expr = (if_expr_t *) malloc(sizeof(if_expr_t));
    expr->expr_type = EXPR_IF;
    expr->data = data;
    return (expr_t *) expr;
}

static expr_t *parse_bool(parser_t *parser) {
    bool_expr_t *expr = (bool_expr_t *) malloc(sizeof(bool_expr_t));
    expr->expr_type = EXPR_BOOL;
    expr->data = is_keyword(current_token, KEYWORD_TRUE);

    advance();
    return (expr_t *) expr;
}

static expr_t *parse_primary(parser_t *parser) {
    skip_end_of_statements(parser);

    switch (current_token->type) {
        case TOKEN_IDENTIFIER:
            return parse_identifier(parser);
        case TOKEN_INTEGER:
            return parse_int(parser);
        default:
            break;
    }

    if (is_keyword(current_token, KEYWORD_TRUE) || is_keyword(current_token, KEYWORD_FALSE)) {
        return parse_bool(parser);
    }

    if (is_keyword(current_token, KEYWORD_IF)) {
        return parse_if(parser);
    }

    if (is_char(current_token, L'(')) {
        advance();
        expr_t *value = parse_expr(parser);

        if (!is_char(current_token, L')')) {
            expected(L"')' after parenthesis expression!\n");
        }
        advance();

        return value;
    }

    fprintf(stderr, "Unexpected token!\n");
    return NULL;
}

static int get_precedence(parser_t *parser) {
    if (current_token->type != TOKEN_OPERATOR) return -1;
    wchar_t *op = get_operator();

    size_t i;
    for (i = 0; i < operator_count; i++) {
        if (!wcscmp(op, operator_precedences[i].op)) {
            return operator_precedences[i].precedence;
        }
    }

    return -1;
}

static expr_t *parse_binary_expr_rhs(parser_t *parser, expr_t *lhs, int precedence) {
    while (1) {
        int token_precedence = get_precedence(parser);
        if (token_precedence < precedence) return lhs;

        wchar_t *op = get_operator();
        advance();

        if (!wcscmp(L"to", op)) {
            assert_is_identifier();

            cast_expr_data_t *data = (cast_expr_data_t *) malloc(sizeof(cast_expr_data_t));
            data->value = lhs;
            data->type = get_identifier();
            advance();

            lhs = (expr_t *) malloc(sizeof(cast_expr_t));
            lhs->expr_type = EXPR_CAST;
            lhs->data = data;
            continue;
        }

        expr_t *rhs = parse_primary(parser);
        if (rhs == NULL) return NULL;

        int next_precedence = get_precedence(parser);
        if (token_precedence < next_precedence) {
            rhs = parse_binary_expr_rhs(parser, rhs, token_precedence + 1);
        }

        binary_expr_t *binary_expr = (binary_expr_t *) malloc(sizeof(binary_expr_t));
        binary_expr->expr_type = EXPR_BINARY;
        binary_expr->data = (binary_expr_data_t *) malloc(sizeof(binary_expr_data_t));
        binary_expr->data->op = op;
        binary_expr->data->lhs = lhs;
        binary_expr->data->rhs = rhs;

        lhs = (expr_t *) binary_expr;
    }
}

static expr_t *parse_expr(parser_t *parser) {
    expr_t *lhs = parse_primary(parser);

    return parse_binary_expr_rhs(parser, lhs, 0);
}

static int is_assignment_stmt_candidate(expr_t *expr) {
    if (expr->expr_type != EXPR_BINARY) return 0;

    binary_expr_data_t *expr_data = (binary_expr_data_t *) expr->data;

    if (wcscmp(L"=", expr_data->op) != 0) return 0;
    return expr_data->lhs->expr_type == EXPR_VARIABLE;
}

static stmt_t *make_assignment_stmt(wchar_t *var_name, expr_t *value) {
    assignment_stmt_data_t *data = (assignment_stmt_data_t *) malloc(sizeof(assignment_stmt_data_t *));
    data->name = var_name;
    data->value = value;

    assignment_stmt_t *stmt = (assignment_stmt_t *) malloc(sizeof(assignment_stmt_t));
    stmt->stmt_type = STMT_ASSIGNMENT;
    stmt->data = data;

    return (stmt_t *) stmt;
}

static stmt_t *parse_declaration(parser_t *parser, int is_var) {
    advance();
    assert_is_identifier();
    wchar_t *var_name = get_identifier();
    advance();

    if (!is_char(current_token, L':')) {
        expected(L"type for variable declaration");
        return NULL;
    }

    advance();
    assert_is_identifier();
    wchar_t *var_type = get_identifier();
    advance();

    typed_ast_value_t *var = (typed_ast_value_t *) malloc(sizeof(typed_ast_value_t));
    var->name = var_name;
    var->type = var_type;
    var->flags = is_var ? VAR_IS_MUTABLE : VAR_IS_IMMUTABLE;

    ptr_list_push(parser->current_function->data->variables, var);

    if (current_token->type == TOKEN_END_OF_STATEMENT) {
        // let has to have a value!
        if (!is_var) expected(L"value for immutable variable");

        return parse_stmt(parser);
    }

    if (!is_operator(current_token, L"=")) {
        expected(L"end of statement or '=' after variable declaration");
    }

    advance();
    expr_t *value = parse_expr(parser);

    stmt_t *ass_stmt = make_assignment_stmt(var_name, value);

    return ass_stmt;
}

static stmt_t *make_assignment_stmt_from_expr(expr_t *expr) {
    binary_expr_data_t *bin_expr_data = (binary_expr_data_t *) expr->data;
    variable_expr_t *var_expr = (variable_expr_t *) bin_expr_data->lhs;
    return make_assignment_stmt(var_expr->name, bin_expr_data->rhs);
}

static stmt_t *parse_while(parser_t *parser) {
    advance();

    expr_t *condition = parse_expr(parser);
    if (condition == NULL) return NULL;

    ptr_list_t *body = parse_body(parser);

    while_stmt_data_t *data = (while_stmt_data_t *) malloc(sizeof(while_stmt_data_t));
    data->condition = condition;
    data->body = body;

    while_stmt_t *stmt = (while_stmt_t *) malloc(sizeof(while_stmt_t));
    stmt->stmt_type = STMT_WHILE;
    stmt->data = data;

    return (stmt_t *) stmt;
}

static stmt_t *parse_stmt(parser_t *parser) {
    if (is_keyword(current_token, KEYWORD_VAR)) {
        return parse_declaration(parser, 1);
    } else if (is_keyword(current_token, KEYWORD_LET)) {
        return parse_declaration(parser, 0);
    }

    if (is_keyword(current_token, KEYWORD_WHILE)) {
        return parse_while(parser);
    }

    stmt_type_t stmt_type = STMT_EXPR;

    if (is_keyword(current_token, KEYWORD_RETURN)) {
        stmt_type = STMT_RETURN;
        advance();
    }

    expr_t *expr = parse_expr(parser);
    if (expr == NULL) return NULL;

    if (is_assignment_stmt_candidate(expr)) {
        return make_assignment_stmt_from_expr(expr);
    }

    stmt_t *stmt = (stmt_t *) malloc(sizeof(stmt_t));
    stmt->stmt_type = stmt_type;
    stmt->data = expr;
    return stmt;
}

static ptr_list_t *parse_body(parser_t *parser) {
    if (!is_char(current_token, L'{')) {
        fprintf(stderr, "Expected '{' to open block\n");
        return NULL;
    }

    advance();
    skip_end_of_statements(parser);

    ptr_list_t *stmts = ptr_list_new();

    while (!is_char(current_token, L'}')) {
        stmt_t *stmt = parse_stmt(parser);
        if (stmt == NULL) return NULL;
        ptr_list_push(stmts, stmt);

        assert_token_type(TOKEN_END_OF_STATEMENT, L"end of statement");
        advance();
    }

    advance();

    return stmts;
}

static stmt_t *parse_function(parser_t *parser) {
    if (!is_keyword(current_token, KEYWORD_FUNCTION)) {
        fprintf(stderr, "Expected function keyword!\n");
        return NULL;
    }

    advance();
    prototype_t *prototype = parse_prototype(parser, 0);

    function_stmt_data_t *data = (function_stmt_data_t *) malloc(sizeof(function_stmt_data_t));
    data->prototype = prototype;
    data->variables = ptr_list_new();

    function_stmt_t *stmt = (function_stmt_t *) malloc(sizeof(function_stmt_t));
    stmt->stmt_type = STMT_FUNCTION;
    stmt->data = data;

    parser->current_function = stmt;

    ptr_list_t *body = parse_body(parser);
    data->body = body;

    parser->current_function = NULL;

    return (stmt_t *) stmt;
}

static stmt_t *parse_extern(parser_t *parser) {
    if (!is_keyword(current_token, KEYWORD_EXTERN)) {
        fprintf(stderr, "Expected extern keyword!");
        return NULL;
    }

    advance();
    prototype_t *prototype = parse_prototype(parser, 1);

    assert_token_type(TOKEN_END_OF_STATEMENT, L"end of statement after extern declaration");
    advance();

    extern_stmt_t *stmt = (extern_stmt_t *) malloc(sizeof(extern_stmt_t));
    stmt->stmt_type = STMT_EXTERN;
    stmt->prototype = prototype;
    return (stmt_t *) stmt;
}

static stmt_t *parse_top_level_stmt(parser_t *parser) {
    if (is_keyword(current_token, KEYWORD_FUNCTION)) {
        return parse_function(parser);
    }

    if (is_keyword(current_token, KEYWORD_EXTERN)) {
        return parse_extern(parser);
    }

    expected(L"function or extern keyword for top level statement");
    return NULL;
}

ptr_list_t *parser_parse_all(parser_t *parser) {
    ptr_list_t *stmts = ptr_list_new();

    while (current_token != NULL) {
        stmt_t *stmt = parse_top_level_stmt(parser);
        if (stmt == NULL) return NULL;

        ptr_list_push(stmts, stmt);

        skip_end_of_statements(parser);
    }

    return stmts;
}
