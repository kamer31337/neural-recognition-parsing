#ifndef SPATTER_OPTION_H
#define SPATTER_OPTION_H

#include "spatter/common.h"
#include <stdbool.h>

typedef struct Option {
    bool is_some;
    const void* value;
} Option;

Option option_some(const void* value);
Option option_none(void);
bool option_is_some(Option opt);
bool option_is_none(Option opt);
const void* option_unwrap_or(Option opt, const void* default_val);
Option option_map(Option opt, const void* (*map_fn)(const void*));
Option option_and_then(Option opt, Option (*and_then_fn)(const void*));

#endif /* SPATTER_OPTION_H */
