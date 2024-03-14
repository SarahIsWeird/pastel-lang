//
// Created by sarah on 3/12/24.
//

#include "ptr_list.h"

#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_CAPACITY 64

struct ptr_list_t {
    void **ptrs;
    size_t size;
    size_t capacity;
    size_t iter_pos;
};

void ensure_capacity(ptr_list_t *list) {
    if (list->size < list->capacity) return;

    list->capacity *= 2;
    void **new_ptrs = (void **) realloc(list->ptrs, sizeof(void *) * list->capacity);
    if (new_ptrs == NULL) {
        fprintf(stderr, "Failed to realloc pointer list!\n");
        exit(1);
    }

    list->ptrs = new_ptrs;
}

ptr_list_t *ptr_list_new() {
    return ptr_list_new_capacity(DEFAULT_CAPACITY);
}

ptr_list_t *ptr_list_new_capacity(size_t capacity) {
    ptr_list_t *list = (ptr_list_t *) malloc(sizeof(ptr_list_t));

    list->ptrs = (void **) malloc(sizeof(void *) * capacity);
    list->size = 0;
    list->capacity = capacity;
    list->iter_pos = 0;

    return list;
}

void ptr_list_free(ptr_list_t *list) {
    free(list->ptrs);
    free(list);
}

void ptr_list_push(ptr_list_t *list, void *ptr) {
    ensure_capacity(list);

    list->ptrs[list->size++] = ptr;
}

size_t ptr_list_size(ptr_list_t *list) {
    return list->size;
}

void *ptr_list_at(ptr_list_t *list, size_t i) {
    if (i >= list->size) return NULL;

    return list->ptrs[i];
}

void *ptr_list_get(ptr_list_t *list) {
    return ptr_list_at(list, list->iter_pos);
}

int ptr_list_next(ptr_list_t *list) {
    if (list->iter_pos == list->size) return 0;

    list->iter_pos++;
    return 1;
}

int ptr_list_previous(ptr_list_t *list) {
    if (list->iter_pos <= 0) return 0;

    list->iter_pos--;
    return 1;
}

void **ptr_list_raw(ptr_list_t *list) {
    return list->ptrs;
}
