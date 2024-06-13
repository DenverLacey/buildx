#include "argiter.h"
#include "cmd.h"
#include "conf.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <unistd.h>

#define TWINE_IMPLEMENTATION
#include "twine.h"

typedef enum RunMode {
    RM_DEBUG,
    RM_RELEASE
} RunMode;

typedef struct CmdRunData {
    RunMode mode;
} CmdRunData;

static void usage_run(void) {
    printf("Usage: bx run [-h] [-d|-r]\n");
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

    static const char *conf_path = BUILDX_DIR "/conf.ini";
    Conf conf;
    if (!read_conf(conf_path, &conf)) {
        logprint(LOG_FATAL, "Couldn't read conf.ini file at '%s'.", conf_path);
        return false;
    }

    if (!version_is_current(conf.buildx.major, conf.buildx.minor)) {
        logprint(LOG_WARN, "Mismatched version %d.%d.%d. This version is %d.%d.%d.\n",
            conf.buildx.major,
            conf.buildx.minor,
            conf.buildx.patch,
            MAJOR_VERSION,
            MINOR_VERSION,
            PATCH_VERSION
        );
    }

    ok = process_options(args, &cmd_data);
    if (!ok) {
        return false;
    }

    char cmd_buf[2048];
    twStringBuf cmdbuf = twStaticBuf(cmd_buf);

    const char *mode_str = cmd_data.mode == RM_RELEASE ? "release" : "debug";

    char exe_path[PATH_MAX];
    snprintf(exe_path, sizeof(exe_path), "%s/%s/%s", conf.proj.out_dir, mode_str, conf.proj.exe_name);

    if (access(exe_path, F_OK) != 0) {
        logprint(LOG_ERROR, "No executable. Use `bs build --%s` first.", mode_str);
        return false;
    }

    if (!twAppendASCII(&cmdbuf, twStr(exe_path))) {
        logprint(LOG_FATAL, "Failed to append executable path to command.");
        return false;
    }

    if (iter_match(args, "--")) {
        while (args->length > 0) {
            const char *arg = iter_next(args);
            if (!twAppendFmtUTF8(&cmdbuf, " %s", arg)) {
                logprint(LOG_FATAL, "Failed to append argument to command.");
                return false;
            }
        }
    }

    twPushASCII(&cmdbuf, '\0');

    twString cmd_str = twBufToString(cmdbuf);
    const char *cmd = cmd_str.bytes;

    if (system(cmd) == -1) {
        logprint(LOG_FATAL, "Failed to run executable '%s'.", exe_path);
        return false;
    }

    return true;
}

