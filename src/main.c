#include "argiter.h"
#include "commands/cmd.h"
#include "utils.h"
#include <stdio.h>

void usage(void) {
    printf("Usage: bx {new,build,run,help} ...\n");
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

    const char *bx_path = iter_next(&args);
    if (!bx_path) {
        logprint(LOG_FATAL, "No bx path passed.");
        return 0;
    }

    if (args.length == 0) {
        usage();
        return 0;
    }

    if (iter_match(&args, "new")) {
        cmd_new(&args, bx_path);
    } else if (iter_match(&args, "build")) {
        cmd_build(&args);
    } else if (iter_match(&args, "run")) {
        cmd_run(&args);
    } else if (iter_match(&args, "help")) {
        usage();
    } else {
        const char *unknown = iter_next(&args);
        logprint(LOG_ERROR, "'%s' is not a valid command.", unknown);
        usage();
    }

    return 0;
}

