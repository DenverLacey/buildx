#include "cmd.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum RunMode {
    RM_DEBUG,
    RM_RELEASE
} RunMode;

typedef struct CmdRunData {
    RunMode mode;
} CmdRunData;

static void usage_run(void) {
    printf("USAGE: bx run [-h] [-d|-r]\n");
    printf("Options:\n");
    printf("    -d, --debug:     Run debug executable.\n");
    printf("    -r, --release:   Run release executable.\n");
    printf("    -h, --help:      Show this help message.\n");
}

static bool cmd_run_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_run();
    exit(0);
    return true;
}

static bool cmd_run_debug(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdRunData *build_data = (CmdRunData *)cmd_data;
    build_data->mode = RM_DEBUG;

    return true;
}

static bool cmd_run_release(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdRunData *build_data = (CmdRunData *)cmd_data;
    build_data->mode = RM_RELEASE;

    return true;
}

static const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_run_help
    },
    (CmdFlagInfo){
        .short_name = "d",
        .long_name = "debug",
        .cmd = cmd_run_debug
    },
    (CmdFlagInfo){
        .short_name = "r",
        .long_name = "release",
        .cmd = cmd_run_release
    },
};

static const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

static bool process_options(ArgIter *args, CmdRunData *cmd_data) {
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                bool ok = flag->cmd(args, cmd_data);
                if (!ok) {
                    usage_run();
                    return false;
                }
            }
        }

        if (!flag_found) {
            break;
        }
    }

    return true;
}

bool cmd_run(ArgIter *args) {
    bool ok;
    CmdRunData cmd_data = {0};

    ok = process_options(args, &cmd_data);
    if (!ok) {
        return false;
    }

    switch (cmd_data.mode) {
        case RM_DEBUG:
            if (system(BUILDX_DIR "/run_debug.sh") == -1) {
                logprint(LOG_FATAL, "Failed to execute run script 'build/run_debug.sh'.");
                return false;
            }
            break;
        case RM_RELEASE:
            if (system(BUILDX_DIR "/run_release.sh") == -1) {
                logprint(LOG_FATAL, "Failed to execute run script 'build/run_release.sh'.");
                return false;
            }
            break;
    }

    return true;
}

