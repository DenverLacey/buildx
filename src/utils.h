#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>

#define UNUSED(...) \
    _Pragma("clang diagnostic push")                                           \
    _Pragma("clang diagnostic ignored \"-Wunused-value\"")                     \
    ((void)(__VA_ARGS__))                                                      \
    _Pragma("clang diagnostic pop")

bool is_short(const char *flag);
bool is_long(const char *flag);

#endif

