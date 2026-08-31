#include "spatter/option.h"

Option option_some(const void* value)
{
    Option opt = { .is_some = true, .value = value };
    return opt;
}

Option option_none(void)
{
    Option opt = { .is_some = false, .value = NULL };
    return opt;
}

bool option_is_some(Option opt)
{
    return opt.is_some;
}

bool option_is_none(Option opt)
{
    return !opt.is_some;
}

const void* option_unwrap_or(Option opt, const void* default_val)
{
    return opt.is_some ? opt.value : default_val;
}

Option option_map(Option opt, const void* (*map_fn)(const void*))
{
    if (!opt.is_some || !map_fn) {
        return option_none();
    }
    return option_some(map_fn(opt.value));
}

Option option_and_then(Option opt, Option (*and_then_fn)(const void*))
{
    if (!opt.is_some || !and_then_fn) {
        return option_none();
    }
    return and_then_fn(opt.value);
}
