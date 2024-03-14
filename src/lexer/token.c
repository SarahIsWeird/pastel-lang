//
// Created by sarah on 3/7/24.
//

#include <malloc.h>
#include <wchar.h>
#include "lexer/token.h"

token_t *token_new(token_type_t type, token_pos_t token_pos) {
    token_t *token = (token_t *) malloc(sizeof(token_t));

    token->type = type;
    token->token_pos = token_pos;

    return token;
}

static token_t *new_string_token(token_type_t type, const wchar_t *start, size_t length, token_pos_t token_pos) {
    token_t *token = token_new(type, token_pos);

    token->data = malloc((length + 1) * sizeof(wchar_t));
    wcsncpy((wchar_t *) token->data, start, length);
    ((wchar_t *) token->data)[length] = 0;

    return token;
}

token_identifier_t *token_new_identifier(const wchar_t *start, size_t length, token_pos_t token_pos) {
    return (token_identifier_t *) new_string_token(TOKEN_IDENTIFIER, start, length, token_pos);
}

token_integer_t *token_new_integer(int value, token_pos_t token_pos) {
    token_integer_t *token = (token_integer_t *) token_new(TOKEN_INTEGER, token_pos);

    token->value = value;

    return token;
}

token_char_t *token_new_char(wchar_t value, token_pos_t token_pos) {
    token_char_t *token = (token_char_t *) token_new(TOKEN_CHAR, token_pos);

    token->value = value;

    return token;
}

token_keyword_t *token_new_keyword(keyword_t keyword, token_pos_t token_pos) {
    token_keyword_t *token = (token_keyword_t *) token_new(TOKEN_KEYWORD, token_pos);

    token->keyword = keyword;

    return token;
}

token_operator_t *token_new_operator(const wchar_t *start, size_t length, token_pos_t token_pos) {
    return (token_operator_t *) new_string_token(TOKEN_OPERATOR, start, length, token_pos);
}

void token_free(token_t *token) {
    if (token->type == TOKEN_IDENTIFIER) {
        wchar_t *identifier = ((token_identifier_t *) token)->value;
        free(identifier);
    }

    free(token);
}

static const wchar_t *DEBUG_keyword_str(keyword_t keyword) {
    switch (keyword) {
        case KEYWORD_FUNCTION:
            return L"function";
        case KEYWORD_IF:
            return L"if";
        case KEYWORD_ELSE:
            return L"else";
        case KEYWORD_FOR:
            return L"for";
        case KEYWORD_LET:
            return L"let";
        case KEYWORD_VAR:
            return L"var";
        case KEYWORD_TRUE:
            return L"true";
        case KEYWORD_FALSE:
            return L"false";
        case KEYWORD_RETURN:
            return L"return";
        case KEYWORD_EXTERN:
            return L"extern";
        case KEYWORD_WHILE:
            return L"while";
    }
}

void DEBUG_token_print(token_t *token) {
    switch (token->type) {
        case TOKEN_NULL:
            wprintf(L"Null lexer\n");
            break;
        case TOKEN_IDENTIFIER:
            wprintf(L"Identifier: [%ls]\n", ((token_identifier_t *) token)->value);
            break;
        case TOKEN_INTEGER:
            wprintf(L"Integer: [%d]\n", ((token_integer_t *) token)->value);
            break;
        case TOKEN_CHAR:
            wprintf(L"Char: [%c]\n", ((token_char_t *) token)->value);
            break;
        case TOKEN_KEYWORD:
            wprintf(L"Keyword: [%ls]\n", DEBUG_keyword_str(((token_keyword_t *) token)->keyword));
            break;
        case TOKEN_OPERATOR:
            wprintf(L"Operator: [%ls]\n", ((token_operator_t *) token)->op);
            break;
        case TOKEN_END_OF_STATEMENT:
            wprintf(L"End of statement\n");
            break;
        default:
            wprintf(L"Unknown token type!\n");
            break;
    }
}
