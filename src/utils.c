#include "utils.h"
#include <ctype.h>

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

