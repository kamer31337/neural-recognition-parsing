#include "spatter/str_view.h"
#include <ctype.h>

StrView str_view_create(const char* data, size_t length)
{
    StrView sv = { .data = data, .length = length };
    return sv;
}

StrView str_view_from_cstr(const char* cstr)
{
    if (!cstr) {
        StrView empty = { .data = NULL, .length = 0 };
        return empty;
    }
    StrView sv = { .data = cstr, .length = strlen(cstr) };
    return sv;
}

bool str_view_is_empty(StrView sv)
{
    return sv.length == 0 || sv.data == NULL;
}

bool str_view_equals(StrView a, StrView b)
{
    if (a.length != b.length) {
        return false;
    }
    if (a.length == 0) {
        return true;
    }
    return memcmp(a.data, b.data, a.length) == 0;
}

bool str_view_equals_cstr(StrView a, const char* cstr)
{
    if (!cstr) {
        return a.length == 0;
    }
    size_t len = strlen(cstr);
    if (a.length != len) {
        return false;
    }
    return memcmp(a.data, cstr, len) == 0;
}

int str_view_compare(StrView a, StrView b)
{
    size_t min_len = SPATTER_MIN(a.length, b.length);
    int cmp = 0;
    if (min_len > 0 && a.data && b.data) {
        cmp = memcmp(a.data, b.data, min_len);
    }
    if (cmp != 0) {
        return cmp;
    }
    if (a.length < b.length) {
        return -1;
    }
    if (a.length > b.length) {
        return 1;
    }
    return 0;
}

StrView str_view_trim(StrView sv)
{
    if (sv.length == 0 || !sv.data) {
        return sv;
    }
    size_t start = 0;
    while (start < sv.length && isspace((unsigned char)sv.data[start])) {
        start++;
    }
    size_t end = sv.length;
    while (end > start && isspace((unsigned char)sv.data[end - 1])) {
        end--;
    }
    return str_view_create(sv.data + start, end - start);
}

StrView str_view_slice(StrView sv, size_t start, size_t length)
{
    if (start >= sv.length || !sv.data) {
        return str_view_create("", 0);
    }
    size_t actual_length = SPATTER_MIN(length, sv.length - start);
    return str_view_create(sv.data + start, actual_length);
}

char* str_view_to_cstr(StrView sv)
{
    char* s = (char*)spatter_alloc(sv.length + 1);
    if (sv.length > 0 && sv.data) {
        memcpy(s, sv.data, sv.length);
    }
    s[sv.length] = '\0';
    return s;
}

void str_view_print(StrView sv, FILE* stream)
{
    if (sv.length > 0 && sv.data) {
        fprintf(stream, SV_FMT, SV_ARG(sv));
    }
}
