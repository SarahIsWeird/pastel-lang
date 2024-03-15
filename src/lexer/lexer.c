//
// Created by sarah on 3/12/24.
//

#include "lexer/lexer.h"
#include "lexer/token.h"

#include <stdlib.h>
#include <stdbool.h>
#include <wchar.h>
#include <wctype.h>

struct lexer_t {
    const wchar_t *input;
    size_t size;
    size_t position;
    token_pos_t current_token_pos;
    ptr_list_t *lexed_tokens;
    int done;
};

typedef struct keyword_type_t {
    const wchar_t *str;
    keyword_t value;
} keyword_type_t;

static keyword_type_t keywords[] = {
        { L"func", KEYWORD_FUNCTION },
        { L"if", KEYWORD_IF },
        { L"else", KEYWORD_ELSE },
        { L"for", KEYWORD_FOR },
        { L"let", KEYWORD_LET },
        { L"var", KEYWORD_VAR },
        { L"true", KEYWORD_TRUE },
        { L"false", KEYWORD_FALSE },
        { L"return", KEYWORD_RETURN },
        { L"extern", KEYWORD_EXTERN },
        { L"while", KEYWORD_WHILE },
};

static size_t keyword_count = sizeof(keywords) / sizeof(keyword_type_t);

static wchar_t *operators[] = {
        L"==",
        L"!=",
        L"<",
        L"<=",
        L">",
        L">=",
        L"+",
        L"-",
        L"*",
        L"/",
        L"to",
};

static size_t operator_count = sizeof(operators) / sizeof(wchar_t *);

#define is_eof() (lexer->size == lexer->position)
#define current_char() (lexer->input[lexer->position])
#define next_char() (lexer->input[lexer->position + 1])
#define advance()   do { \
                        lexer->current_token_pos.column++; \
                        if (current_char() == '\n') { \
                            lexer->current_token_pos.column = 0; \
                            lexer->current_token_pos.line++; \
                        } \
                        lexer->position++; \
                    } while (0)

static void skip_whitespace(lexer_t *lexer) {
    while (true) {
        if (is_eof()) return;

        wchar_t c = current_char();
        if (!iswblank(c)) return;

        advance();
    }
}

static void skip_until_newline(lexer_t *lexer) {
    while (true) {
        if (is_eof()) return;

        wchar_t ch = current_char();
        if (ch == L'\r') {
            advance();

            ch = current_char();
            if (ch == L'\n') advance();

            return;
        }

        if (ch == L'\n') {
            advance();
            return;
        }

        advance();
    }
}

static token_operator_t *get_operator(const wchar_t *start, size_t length, token_pos_t token_pos) {
    size_t i;
    for (i = 0; i < operator_count; i++) {
        if (!wcsncmp(operators[i], start, length)) {
            return token_new_operator(start, length, token_pos);
        }
    }

    return NULL;
}

#define max(a, b) (a > b ? a : b)

static token_keyword_t *get_keyword(const wchar_t *start, size_t length, token_pos_t token_pos) {
    size_t i;
    for (i = 0; i < keyword_count; i++) {
        keyword_type_t *type = &keywords[i];

        if (!wcsncmp(type->str, start, max(length, wcslen(type->str)))) {
            return token_new_keyword(type->value, token_pos);
        }
    }

    return NULL;
}

static token_t *lex_identifier(lexer_t *lexer) {
    const wchar_t *start = &current_char();
    size_t length = 0;

    do {
        length++;
        advance();
    } while (iswalnum(current_char()) || current_char() == L'_');

    token_pos_t pos = lexer->current_token_pos;
    pos.column -= length;

    token_keyword_t *keyword_token = get_keyword(start, length, pos);
    if (keyword_token != NULL) {
        return (token_t *) keyword_token;
    }

    token_operator_t *operator_token = get_operator(start, length, pos);
    if (operator_token != NULL) {
        return (token_t *) operator_token;
    }

    return (token_t *) token_new_identifier(start, length, pos);
}

static token_t *lex_number(lexer_t *lexer) {
    token_pos_t pos = lexer->current_token_pos;
    const wchar_t *start = lexer->input + lexer->position;
    wchar_t *end;
    int int_value = (int) wcstol(start, &end, 10);

    size_t offset = end - start;
    lexer->position += offset;

    if (current_char() != L'.') {
        lexer->current_token_pos.column += offset;
        return (token_t *) token_new_integer(int_value, pos);
    }

    lexer->position -= offset; // Rewind to the start of the number
    double double_value = wcstod(start, &end);

    offset = end - start;
    lexer->position += offset;
    lexer->current_token_pos.column += offset;

    return (token_t *) token_new_float(double_value, pos);
}

static int is_operator_starting(lexer_t *lexer) {
    size_t i;
    for (i = 0; i < operator_count; i++) {
        if (current_char() == *operators[i]) return 1;
    }

    return 0;
}

static token_t *lex_operator(lexer_t *lexer) {
    const wchar_t *start = &current_char();
    size_t length = 1;

    while (true) {
        bool found = false;
        size_t i;

        for (i = 0; i < operator_count; i++) {
            if (!wcsncmp(start, operators[i], length)) {
                advance();
                length++;
                found = true;
                break;
            }
        }

        if (!found) break;
    }

    length--;

    return (token_t *) token_new_operator(start, length, lexer->current_token_pos);
}

static int is_end_of_statement(lexer_t *lexer) {
    wchar_t c = current_char();

    return (c == L'\r' || c == L'\n' || c == L';');
}

static void skip_consecutive_end_of_statements(lexer_t *lexer) {
    while (is_end_of_statement(lexer)) {
        advance();
    }
}

static void lex_next(lexer_t *lexer) {
    skip_whitespace(lexer);
    if (is_eof()) return;

    // Comments
    if ((current_char() == L'/') && (next_char() == L'/')) {
        skip_until_newline(lexer);
        return;
    }

    if (is_end_of_statement(lexer)) {
        skip_consecutive_end_of_statements(lexer);
        token_t *token = token_new(TOKEN_END_OF_STATEMENT, lexer->current_token_pos);
        ptr_list_push(lexer->lexed_tokens, token);
        return;
    }

    if (iswalpha(current_char())) {
        token_t *token = lex_identifier(lexer);
        ptr_list_push(lexer->lexed_tokens, token);
        return;
    }

    if (iswdigit(current_char()) || ((current_char() == L'-') && (iswdigit(next_char())))) {
        token_t *token = lex_number(lexer);
        ptr_list_push(lexer->lexed_tokens, token);
        return;
    }

    if (is_operator_starting(lexer)) {
        token_t *token = lex_operator(lexer);
        ptr_list_push(lexer->lexed_tokens, token);
        return;
    }

    token_t *token = (token_t *) token_new_char(current_char(), lexer->current_token_pos);
    ptr_list_push(lexer->lexed_tokens, token);
    advance();
}

lexer_t *lexer_new(const wchar_t *input, size_t size) {
    lexer_t *lexer = (lexer_t *) malloc(sizeof(lexer_t));

    lexer->input = input;
    lexer->size = size;
    lexer->position = 0;
    lexer->lexed_tokens = ptr_list_new();
    lexer->done = false;
    lexer->current_token_pos.line = 1;
    lexer->current_token_pos.column = 1;

    return lexer;
}

void lexer_lex_all(lexer_t *lexer) {
    while (!is_eof()) {
        lex_next(lexer);
    }

    lexer->done = true;
}

ptr_list_t *lexer_get_tokens(lexer_t *lexer) {
    if (!lexer->done) return NULL;

    return lexer->lexed_tokens;
}
