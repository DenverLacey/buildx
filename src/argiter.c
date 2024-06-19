#include "argiter.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

ArgIter iter_create(int argc, const char **argv) {
    return (ArgIter){
        .length = argc,
        .args = argv
    };
}

const char *iter_peek(ArgIter *iter) {
    if (iter->length == 0) {
        return NULL;
    }

    return iter->args[0];
}

const char *iter_next(ArgIter *iter) {
    if (iter->length == 0) {
        return NULL;
    }

    iter->length--;
    return *iter->args++;
}

void iter_back(ArgIter *iter) {
    iter->length++;
    iter->args--;
}

bool iter_match(ArgIter *iter, const char *arg) {
    if (iter->length == 0) {
        return false;
    }

    if (strcmp(iter->args[0], arg) != 0) {
        return false;
    }

    iter_next(iter);
    return true;
}

bool iter_check_flags(ArgIter *iter, const char *const *flags) {
    const char *arg = iter_peek(iter);
    if (!arg) {
        return false;
    }

    if (is_short(arg)) {
        for (const char *c = &arg[1]; *c != '\0'; c++) {
            if (*c == flags[0][0]) {
                return true;
            }
        }
        return false;
    } else if (is_long(arg)) {
        return strcmp(&arg[2], flags[1]) == 0;
    } else {
        return false;
    }
}

