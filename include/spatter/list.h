#ifndef SPATTER_LIST_H
#define SPATTER_LIST_H

#include "spatter/common.h"

typedef struct List {
    const void* head;
    const struct List* tail;
} List;

const List* list_empty(void);
const List* list_cons(const void* head, const List* tail);
bool list_is_empty(const List* list);
const void* list_head(const List* list);
const List* list_tail(const List* list);
size_t list_length(const List* list);
const List* list_reverse(const List* list);
const List* list_append(const List* a, const List* b);
const List* list_map(const List* list, const void* (*map_fn)(const void*));
const List* list_filter(const List* list, bool (*pred_fn)(const void*));
const void* list_fold_left(const List* list, const void* initial, const void* (*fold_fn)(const void* acc, const void* elem));
const List* list_take(const List* list, size_t n);
const List* list_sort(const List* list, int (*compare_fn)(const void* a, const void* b));
const void* list_nth(const List* list, size_t n);
void list_free_nodes(const List* list);

#endif /* SPATTER_LIST_H */
