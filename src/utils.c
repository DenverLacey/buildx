#include "utils.h"
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>

#define TWINE_IMPLEMENTATION
#include "twine.h"

bool version_is_compatible(int major, int minor) {
    return major == MAJOR_VERSION && minor == MINOR_VERSION;
}

bool version_is_current(int major, int minor, int patch) {
    return major == MAJOR_VERSION && minor == MINOR_VERSION && PATCH_VERSION == patch;
}

bool is_short(const char *flag) {
    if (!flag) return false;
    if (flag[0] == '\0') return false;
    if (flag[1] == '\0') return false;
    return flag[0] == '-' && flag[1] != '-';
}

bool is_long(const char *flag) {
    if (!flag) return false;
    if (flag[0] == '\0') return false;
    if (flag[1] == '\0') return false;
    return flag[0] == '-' && flag[1] == '-';
}

char *strupper(char *s) {
    for (char *c = s; *c != '\0'; c++) {
        *c = toupper(*c);
    }
    return s;
}

bool starts_with(const char *s, const char *prefix) {
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

#define COLOR_RESET "\033[m"
#define COLOR_DEBUG "\033[32m"
#define COLOR_INFO  "\033[36m"
#define COLOR_WARN  "\033[33m"
#define COLOR_ERROR "\033[31m"
#define COLOR_FATAL "\033[31m"

const char *log_level_colors[] = {
    COLOR_RESET,
    COLOR_DEBUG,
    COLOR_INFO,
    COLOR_WARN,
    COLOR_ERROR,
    COLOR_FATAL
};

const char *log_level_labels[] = {
    "",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void logprint(LogLevel lv, const char *__restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
        if (lv != LOG_NONE) {
            printf("%s%s" COLOR_RESET ": ", log_level_colors[lv], log_level_labels[lv]);
        }
        vprintf(fmt, args);
        printf("\n");
    va_end(args);
}

int prompt(const char *__restrict fmt, ...) {
    va_list args;
    va_start(args, fmt);
        printf("%sINFO%s: ", log_level_colors[LOG_INFO], COLOR_RESET);
        vprintf(fmt, args);
        int c = fgetc(stdin);
    va_end(args);
    return c;
}

Dialect dialect_from_str(const char *s) {
    for (int i = 0; i < DIALECT_COUNT; i++) {
        if (strcasecmp(s, dialect_names[i]) == 0) {
            return (Dialect)i;
        }
    }
    return -1;
}

