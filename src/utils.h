#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>

#define MAJOR_VERSION (0)
#define MINOR_VERSION (2)
#define PATCH_VERSION (0)

#define BUILDX_DIR ".buildx"

#define UNUSED(...) \
    _Pragma("clang diagnostic push")                                           \
    _Pragma("clang diagnostic ignored \"-Wunused-value\"")                     \
    ((void)(__VA_ARGS__))                                                      \
    _Pragma("clang diagnostic pop")

bool is_short(const char *flag);
bool is_long(const char *flag);

char *strupper(char *s);

typedef enum LogLevel {
    LOG_NONE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void logprint(LogLevel lv, const char *__restrict fmt, ...);

#endif

