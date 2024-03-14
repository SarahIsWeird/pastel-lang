#ifndef PASTEL_TOKEN_H
#define PASTEL_TOKEN_H

#include <stddef.h>

typedef enum token_type_t {
    TOKEN_NULL = 0,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_CHAR,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_END_OF_STATEMENT,
} token_type_t;

typedef enum keyword_t {
    KEYWORD_FUNCTION,
    KEYWORD_IF,
    KEYWORD_ELSE,
    KEYWORD_FOR,
    KEYWORD_LET,
    KEYWORD_VAR,
    KEYWORD_TRUE,
    KEYWORD_FALSE,
    KEYWORD_RETURN,
    KEYWORD_EXTERN,
} keyword_t;

typedef struct token_pos_t {
    size_t line;
    size_t column;
} token_pos_t;

typedef struct token_t {
    token_type_t type;
    token_pos_t token_pos;
    void *data;
} token_t;

typedef struct token_identifier_t {
    token_type_t type;
    token_pos_t token_pos;
    wchar_t *value;
} token_identifier_t;

typedef struct token_integer_t {
    token_type_t type;
    token_pos_t token_pos;
    int value;
} token_integer_t;

typedef struct token_char_t {
    token_type_t type;
    token_pos_t token_pos;
    wchar_t value;
} token_char_t;

typedef struct token_keyword_t {
    token_type_t type;
    token_pos_t token_pos;
    keyword_t keyword;
} token_keyword_t;

typedef struct token_operator_t {
    token_type_t type;
    token_pos_t token_pos;
    wchar_t *op;
} token_operator_t;

token_t *token_new(token_type_t type, token_pos_t token_pos);
token_identifier_t *token_new_identifier(const wchar_t *start, size_t length, token_pos_t token_pos);
token_integer_t *token_new_integer(int value, token_pos_t token_pos);
token_char_t *token_new_char(wchar_t value, token_pos_t token_pos);
token_keyword_t *token_new_keyword(keyword_t keyword, token_pos_t token_pos);
token_operator_t *token_new_operator(const wchar_t *start, size_t length, token_pos_t token_pos);

void DEBUG_token_print(token_t *token);

void token_free(token_t *token);

#endif //PASTEL_TOKEN_H
