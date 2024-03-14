//
// Created by sarah on 3/12/24.
//

#ifndef PASTEL_PARSER_H
#define PASTEL_PARSER_H

#include "../util/ptr_list.h"

typedef struct parser_t parser_t;

parser_t *parser_new(ptr_list_t *tokens);
void parser_free(parser_t *parser);

// Returns List<stmt_t *> of the top level statements
ptr_list_t *parser_parse_all(parser_t *parser);

#endif //PASTEL_PARSER_H
