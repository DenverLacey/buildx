#include "utils.h"
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>

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

#define COLOR_RESET "\033[m"
#define COLOR_DEBUG   "\033[32m"
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

