#include "cmd.h"

#include "argiter.h"
#include "conf.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <unistd.h>

static void usage_project(void) {
    printf("Usage: \n");
    printf("    bx project [-h]\n");
    printf("    bx project upgrade [-h]\n");
    printf("Subcommands:\n");
    printf("    upgrade:    Upgrades project to the latest version of buildx.\n");
    printf("Options:\n");
    printf("    -h, --help: Show this help message.\n");
}

static bool cmd_project_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_project();
    exit(0);
    return true;
}

typedef struct {
    char _phantom_data;
} CmdProjectData;

static const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_project_help
    },
};

static const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

static bool process_options(ArgIter *args, CmdProjectData *cmd_data) {
    UNUSED(cmd_data);

    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                bool ok = flag->cmd(args, cmd_data);
                if (!ok) {
                    usage_project();
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

static bool cmd_project_upgrade(ArgIter *args, CmdProjectData *cmd_data) {
    const char *build_dir = NULL;
    struct stat s = {0};
    if (stat("./" BUILDX_DIR, &s) != 0) {
        build_dir = BUILDX_DIR;
    } else if (stat("./build", &s) != 0) {
        build_dir = "build";
    } else {
        assert(!"TODO");
    }

    if (!S_ISDIR(s.st_mode)) {
        assert(!"TODO");
    }

    char conf_path[PATH_MAX];
    snprintf(conf_path, sizeof(conf_path), "./%s/conf.ini", build_dir);

    if (access(conf_path, F_OK) != 0) {
        assert(!"TODO");
    }

    Conf conf;
    if (!read_conf(conf_path, &conf)) {
        assert(!"TODO");
    }

    assert(!"TODO");
}

bool cmd_project(ArgIter *args) {
    CmdProjectData cmd_data = {0};

    if (iter_match(args, "upgrade")) {
        if (!process_options(args, &cmd_data)) {
            return false;
        }

        if (!cmd_project_upgrade(args, &cmd_data)) {
            return false;
        }
    } else {
        if (!process_options(args, &cmd_data)) {
            return false;
        }

        const char *conf_path = "./" BUILDX_DIR "/conf.ini";

        if (access(conf_path, F_OK) != 0) {
            logprint(LOG_FATAL, "Could not find conf.ini file at '%s'.", conf_path);
            return false;
        }

        char cmd[PATH_MAX];
        snprintf(cmd, sizeof(cmd), "cat %s", conf_path);

        if (system(cmd) == -1) {
            logprint(LOG_FATAL, "Failed to print conf.ini file.");
            return false;
        }
    }

    return true;
}

