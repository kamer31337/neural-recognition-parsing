#include "spatter/list.h"

const List* list_empty(void)
{
    return NULL;
}

const List* list_cons(const void* head, const List* tail)
{
    List* node = (List*)spatter_alloc(sizeof(List));
    node->head = head;
    node->tail = tail;
    return node;
}

bool list_is_empty(const List* list)
{
    return list == NULL;
}

const void* list_head(const List* list)
{
    return list ? list->head : NULL;
}

const List* list_tail(const List* list)
{
    return list ? list->tail : NULL;
}

size_t list_length(const List* list)
{
    size_t count = 0;
    const List* curr = list;
    while (curr) {
        count++;
        curr = curr->tail;
    }
    return count;
}

const List* list_reverse(const List* list)
{
    const List* acc = list_empty();
    const List* curr = list;
    while (curr) {
        acc = list_cons(curr->head, acc);
        curr = curr->tail;
    }
    return acc;
}

const List* list_append(const List* a, const List* b)
{
    if (!a) {
        return b;
    }
    const List* rev_a = list_reverse(a);
    const List* result = b;
    const List* curr = rev_a;
    while (curr) {
        result = list_cons(curr->head, result);
        curr = curr->tail;
    }
    list_free_nodes(rev_a);
    return result;
}

const List* list_map(const List* list, const void* (*map_fn)(const void*))
{
    if (!list || !map_fn) {
        return list_empty();
    }
    const List* rev_acc = list_empty();
    const List* curr = list;
    while (curr) {
        rev_acc = list_cons(map_fn(curr->head), rev_acc);
        curr = curr->tail;
    }
    const List* result = list_reverse(rev_acc);
    list_free_nodes(rev_acc);
    return result;
}

const List* list_filter(const List* list, bool (*pred_fn)(const void*))
{
    if (!list || !pred_fn) {
        return list_empty();
    }
    const List* rev_acc = list_empty();
    const List* curr = list;
    while (curr) {
        if (pred_fn(curr->head)) {
            rev_acc = list_cons(curr->head, rev_acc);
        }
        curr = curr->tail;
    }
    const List* result = list_reverse(rev_acc);
    list_free_nodes(rev_acc);
    return result;
}

const void* list_fold_left(const List* list, const void* initial, const void* (*fold_fn)(const void* acc, const void* elem))
{
    const void* acc = initial;
    const List* curr = list;
    while (curr) {
        acc = fold_fn(acc, curr->head);
        curr = curr->tail;
    }
    return acc;
}

const List* list_take(const List* list, size_t n)
{
    if (!list || n == 0) {
        return list_empty();
    }
    const List* rev_acc = list_empty();
    const List* curr = list;
    size_t count = 0;
    while (curr && count < n) {
        rev_acc = list_cons(curr->head, rev_acc);
        count++;
        curr = curr->tail;
    }
    const List* result = list_reverse(rev_acc);
    list_free_nodes(rev_acc);
    return result;
}

const void* list_nth(const List* list, size_t n)
{
    const List* curr = list;
    size_t i = 0;
    while (curr) {
        if (i == n) {
            return curr->head;
        }
        i++;
        curr = curr->tail;
    }
    return NULL;
}

const List* list_sort(const List* list, int (*compare_fn)(const void* a, const void* b))
{
    size_t len = list_length(list);
    if (len <= 1) {
        return list;
    }
    const void** arr = (const void**)spatter_alloc(len * sizeof(const void*));
    const List* curr = list;
    size_t idx = 0;
    while (curr) {
        arr[idx++] = curr->head;
        curr = curr->tail;
    }
    for (size_t i = 0; i < len; i++) {
        for (size_t j = i + 1; j < len; j++) {
            if (compare_fn(arr[i], arr[j]) > 0) {
                const void* temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
    const List* sorted = list_empty();
    for (size_t i = len; i > 0; i--) {
        sorted = list_cons(arr[i - 1], sorted);
    }
    spatter_free(arr);
    return sorted;
}

void list_free_nodes(const List* list)
{
    const List* curr = list;
    while (curr) {
        const List* next = curr->tail;
        spatter_free((void*)curr);
        curr = next;
    }
}
