#include "spatter/result.h"

Result result_ok(const void* value)
{
    Result res = { .is_ok = true, .as = { .ok_val = value } };
    return res;
}

Result result_err(const char* error_message)
{
    Result res = { .is_ok = false, .as = { .err_msg = error_message ? error_message : "Unknown error" } };
    return res;
}

bool result_is_ok(Result res)
{
    return res.is_ok;
}

bool result_is_err(Result res)
{
    return !res.is_ok;
}

const void* result_unwrap(Result res)
{
    if (!res.is_ok) {
        fprintf(stderr, "[FATAL] Attempted to unwrap an Err Result: %s\n", res.as.err_msg);
        exit(EXIT_FAILURE);
    }
    return res.as.ok_val;
}

const void* result_unwrap_or(Result res, const void* default_val)
{
    return res.is_ok ? res.as.ok_val : default_val;
}

const char* result_unwrap_err(Result res)
{
    if (res.is_ok) {
        fprintf(stderr, "[FATAL] Attempted to unwrap_err on an Ok Result\n");
        exit(EXIT_FAILURE);
    }
    return res.as.err_msg;
}

Result result_map(Result res, const void* (*map_fn)(const void*))
{
    if (!res.is_ok || !map_fn) {
        return res;
    }
    return result_ok(map_fn(res.as.ok_val));
}

Result result_and_then(Result res, Result (*and_then_fn)(const void*))
{
    if (!res.is_ok || !and_then_fn) {
        return res;
    }
    return and_then_fn(res.as.ok_val);
}
