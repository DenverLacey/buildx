#ifndef _ARGITER_H_
#define _ARGITER_H_

#include <stdbool.h>

typedef struct ArgIter {
	int length;
	const char **args;
} ArgIter;

ArgIter iter_create(int argc, const char **argv);
const char *iter_peek(ArgIter *iter);
const char *iter_next(ArgIter *iter);
bool iter_match(ArgIter *iter, const char *arg);
bool iter_check_flags(ArgIter *iter, const char *const *flags);

#endif

