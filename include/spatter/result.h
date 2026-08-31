#ifndef SPATTER_RESULT_H
#define SPATTER_RESULT_H

#include "spatter/common.h"

typedef struct Result {
    bool is_ok;
    union {
        const void* ok_val;
        const char* err_msg;
    } as;
} Result;

Result result_ok(const void* value);
Result result_err(const char* error_message);
bool result_is_ok(Result res);
bool result_is_err(Result res);
const void* result_unwrap(Result res);
const void* result_unwrap_or(Result res, const void* default_val);
const char* result_unwrap_err(Result res);
Result result_map(Result res, const void* (*map_fn)(const void*));
Result result_and_then(Result res, Result (*and_then_fn)(const void*));

#endif /* SPATTER_RESULT_H */
