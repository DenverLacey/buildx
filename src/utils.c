#include "utils.h"

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

