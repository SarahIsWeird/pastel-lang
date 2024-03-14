//
// Created by sarah on 3/12/24.
//

#ifndef PASTEL_PTR_LIST_H
#define PASTEL_PTR_LIST_H

#include <stddef.h>

typedef struct ptr_list_t ptr_list_t;

ptr_list_t *ptr_list_new();
ptr_list_t *ptr_list_new_capacity(size_t capacity);
void ptr_list_free(ptr_list_t *list);

void ptr_list_push(ptr_list_t *list, void *ptr);

size_t ptr_list_size(ptr_list_t *list);
void *ptr_list_at(ptr_list_t *list, size_t i);

/* Iterator methods */

// Retrieves the pointer at the current iterator position.
void *ptr_list_get(ptr_list_t *list);

// Advances the iterator forwards once. Returns 0 if called when done.
int ptr_list_next(ptr_list_t *list);

// Advances the iterator backwards once. Returns 0 if called when done.
int ptr_list_previous(ptr_list_t *list);

// Unsafe! Only use to read in conjunction with ptr_list_size.
void **ptr_list_raw(ptr_list_t *list);

#endif //PASTEL_PTR_LIST_H
