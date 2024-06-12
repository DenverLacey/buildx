#include "cmd.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define DEBUG_BUILD_SCRIPT (BUILDX_DIR "/build_debug.sh")
#define RELEASE_BUILD_SCRIPT (BUILDX_DIR "/build_release.sh")

typedef enum BuildMode {
    BM_DEBUG,
    BM_RELEASE
} BuildMode;

typedef struct CmdBuildData {
    BuildMode mode;
} CmdBuildData;

static void usage_build(void) {
    printf("Usage: bx build [-h|-d|-r]\n");
    printf("Options:\n");
    printf("    -d, --debug:     Build debug executable.\n");
    printf("    -r, --release:   Build release executable.\n");
    printf("    -h, --help:      Show this help message.\n");
}

static bool cmd_build_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_build();
    exit(0);
    return true;
}

static bool cmd_build_debug(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdBuildData *build_data = (CmdBuildData *)cmd_data;
    build_data->mode = BM_DEBUG;

    return true;
}

static bool cmd_build_release(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdBuildData *build_data = (CmdBuildData *)cmd_data;
    build_data->mode = BM_RELEASE;

    return true;
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

static bool process_options(ArgIter *args, CmdBuildData *cmd_data) {
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                bool ok = flag->cmd(args, cmd_data);
                if (!ok) {
                    usage_build();
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

bool cmd_build(ArgIter *args) {
    bool ok;
    CmdBuildData cmd_data = {0};

    ok = process_options(args, &cmd_data);
    if (!ok) {
        return false;
    }

    const char *build_cmd = cmd_data.mode == BM_DEBUG ? DEBUG_BUILD_SCRIPT : RELEASE_BUILD_SCRIPT;
    if (system(build_cmd) == -1) {
        logprint(LOG_FATAL, "Failed to run build script '%s'.", build_cmd);
        return false;
    }

    return true;
}

