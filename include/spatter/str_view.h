#ifndef SPATTER_STR_VIEW_H
#define SPATTER_STR_VIEW_H

#include "spatter/common.h"

typedef struct StrView {
    const char* data;
    size_t length;
} StrView;

#define SV_FMT "%.*s"
#define SV_ARG(sv) ((int)(sv).length), ((sv).data ? (sv).data : "")

StrView str_view_create(const char* data, size_t length);
StrView str_view_from_cstr(const char* cstr);
bool str_view_is_empty(StrView sv);
bool str_view_equals(StrView a, StrView b);
bool str_view_equals_cstr(StrView a, const char* cstr);
int str_view_compare(StrView a, StrView b);
StrView str_view_trim(StrView sv);
StrView str_view_slice(StrView sv, size_t start, size_t length);
char* str_view_to_cstr(StrView sv);
void str_view_print(StrView sv, FILE* stream);

#endif /* SPATTER_STR_VIEW_H */
