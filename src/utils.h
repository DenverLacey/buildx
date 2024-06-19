#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdbool.h>

#include "argiter.h"
#include "commands/cmd.h"
#include "twine.h"

#define MAJOR_VERSION (0)    // Changes when incompatible API changes are made.
#define MINOR_VERSION (5)    // Changes when functionality is added in a backwards compatible manner.
#define PATCH_VERSION (0)    // Changes when backwards compatible bug fixes and refactors are made.

bool version_is_compatible(int major, int minor);
bool version_is_current(int major, int minor, int patch);

#define BUILDX_DIR ".buildx"

#define UNUSED(...) \
    _Pragma("clang diagnostic push")                                           \
    _Pragma("clang diagnostic ignored \"-Wunused-value\"")                     \
    ((void)(__VA_ARGS__))                                                      \
    _Pragma("clang diagnostic pop")

#define RETURN(...) do {                                                       \
    result = __VA_ARGS__;						       \
    goto CLEAN_UP_AND_RETURN;                                                  \
} while (0)

bool is_short(const char *flag);
bool is_long(const char *flag);

bool process_options(ArgIter *args, void *cmd_data, const CmdFlagInfo *flags, size_t flags_len);

char *strupper(char *s);
bool starts_with(const char *s, const char *prefix);

typedef enum LogLevel {
    LOG_NONE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

void logprint(LogLevel lv, const char *__restrict fmt, ...);
int prompt(const char *__restrict fmt, ...);

typedef enum Dialect {
    C99 = 0,
    C11,
    C17,
    CPP11,
    CPP14,
    CPP17,
    DIALECT_COUNT
} Dialect;

static const char *dialect_names[DIALECT_COUNT] = {
    "c99",
    "c11",
    "c17",
    "c++11",
    "c++14",
    "c++17"
};

Dialect dialect_from_str(const char *s);

#endif

