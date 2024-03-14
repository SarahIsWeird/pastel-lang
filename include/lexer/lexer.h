//
// Created by sarah on 3/12/24.
//

#ifndef PASTEL_LEXER_H
#define PASTEL_LEXER_H

#include <stddef.h>
#include "../../src/util/ptr_list.h"

typedef struct lexer_t lexer_t;

/*
 * input MUST be null-terminated, because it allows us to skip a lot of checks.
 */
lexer_t *lexer_new(const wchar_t *input, size_t size);

void lexer_lex_all(lexer_t *lexer);
ptr_list_t *lexer_get_tokens(lexer_t *lexer);

#endif //PASTEL_LEXER_H
