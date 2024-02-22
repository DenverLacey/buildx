#include "argiter.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>

#include "commands/cmd.h"

void usage(void) {
    printf("USAGE: bx {new,build,run,help} ...\n");
    printf("    new:   Initialize a new project.\n");
    printf("           Use `bx new --help` for more info.\n");
    printf("    build: Build project.\n");
    printf("           Use `bx build --help` for more info.\n");
    printf("    run:   Run your already built project.\n");
    printf("           Use `bx run --help` for more info.\n");
    printf("    help:  Show this help message.\n");
}

int main(int argc, const char **argv) {
    ArgIter args = iter_create(argc, argv);
    UNUSED(iter_next(&args)); // Skip bin directory

    if (args.length == 0) {
        usage();
        return 0;
    }

    if (iter_match(&args, "new")) {
        cmd_new(&args);
    } else if (iter_match(&args, "build")) {
        cmd_build(&args);
    } else if (iter_match(&args, "run")) {
        cmd_run(&args);
    } else if (iter_match(&args, "help")) {
        usage();
    } else {
        const char *unknown = iter_next(&args);
        logprint(ERROR, "'%s' is not a valid command.", unknown);
        usage();
    }

    return 0;
}

