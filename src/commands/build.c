#include "cmd.h"
#include "errors.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

typedef enum BuildMode {
    BM_DEBUG,
    BM_RELEASE
} BuildMode;

typedef struct CmdBuildData {
    BuildMode mode;
} CmdBuildData;

static void usage_build(void) {
    printf("USAGE: bx build [-h] [-d|-r]\n");
    printf("Options:\n");
    printf("    -d, --debug:     Build debug executable.\n");
    printf("    -r, --release:   Build release executable.\n");
    printf("    -h, --help:      Show this help message.\n");
}

static ResultCode cmd_build_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_build();
    exit(0);
    return OK;
}

static ResultCode cmd_build_debug(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdBuildData *build_data = (CmdBuildData *)cmd_data;
    build_data->mode = BM_DEBUG;

    return OK;
}

static ResultCode cmd_build_release(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdBuildData *build_data = (CmdBuildData *)cmd_data;
    build_data->mode = BM_RELEASE;

    return OK;
}

static const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_build_help
    },
    (CmdFlagInfo){
        .short_name = "d",
        .long_name = "debug",
        .cmd = cmd_build_debug
    },
    (CmdFlagInfo){
        .short_name = "r",
        .long_name = "release",
        .cmd = cmd_build_release
    },
};

static const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

static ResultCode process_options(ArgIter *args, CmdBuildData *cmd_data) {
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                ResultCode result = flag->cmd(args, cmd_data);
                if (result != OK) {
                    usage_build();
                    return result;
                }
            }
        }

        if (!flag_found) {
            break;
        }
    }

    return OK;
}

ResultCode cmd_build(ArgIter *args) {
    ResultCode result;
    CmdBuildData cmd_data = {0};

    result = process_options(args, &cmd_data);
    if (result != OK) {
        return result;
    }

    switch (cmd_data.mode) {
        case BM_DEBUG:
            if (system("build/build_debug.sh") == -1) {
                logprint(LOG_FATAL, "Failed to run build script 'build/build_debug.sh'.");
                return INTERNAL_ERROR;
            }
            break;
        case BM_RELEASE:
            if (system("build/build_release.sh") == -1) {
                logprint(LOG_FATAL, "Failed to run build script 'build/build_release.sh'.");
                return INTERNAL_ERROR;
            }
            break;
    }

    return OK;
}

