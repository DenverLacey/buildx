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
    printf("    bx project SETTING=VALUE\n");
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
    UNUSED(args, cmd_data);

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

// TODO: 
// - [ ] Check that setting is allowed to be changed.
// - [ ] Change settings in premake5.lua where appropriate.
static bool cmd_project_change(ArgIter *args, CmdProjectData *cmd_data, twString setting, twString value) {
    UNUSED(args, cmd_data);

    bool result = true;
    FILE *f = NULL;

    static const char *conf_path = "./.buildx/conf.ini";

    char bufmem[PATH_MAX*2];
    twStringBuf buf = twStaticBuf(bufmem);

    // Read in conf file and change the desired line
    {
        f = fopen(conf_path, "r");
        if (!f) {
            logprint(LOG_FATAL, "Failed to open conf.ini file at '%s'.", conf_path);
            RETURN(false);
        }

        char *line_c_str = NULL;
        size_t linecap = 0;
        ssize_t len = 0;

        while ((len = getline(&line_c_str, &linecap, f)) > 0) {
            twString line = (twString){.bytes = line_c_str, .length = len - 1}; // -1 for newline
            if (twStartsWith(line, setting)) {
                if (!twAppendFmtUTF8(&buf, twFmt" = "twFmt"\n", twArg(setting), twArg(value))) {
                    logprint(LOG_FATAL, "Failed to append line to string buffer: '"twFmt" = "twFmt"'.", twArg(setting), twArg(value));
                    RETURN(false);
                }
            } else {
                if (!twAppendLineUTF8(&buf, line)) {
                    logprint(LOG_FATAL, "Failed to append line to string buffer: '"twFmt"'.", twArg(line));
                    RETURN(false);
                }
            }
        }

        twPushASCII(&buf, '\0'); // guarantee null terminator

        fclose(f);
    }

    // Write changed file to disk
    {
        f = fopen(conf_path, "w");
        if (!f) {
            logprint(LOG_FATAL, "Failed to open conf.ini file at '%s'.", conf_path);
            RETURN(false);
        }

        const char *new_contents = twToC(buf);

        if (fputs(new_contents, f) == EOF) {
            const char *err = strerror(errno);
            logprint(LOG_ERROR, "Failed to update conf.ini file: %s.", err);
            RETURN(false);
        }
    }

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
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

        twString arg = twStr(iter_peek(args));
        twString value;
        twString setting = twSplitUTF8(arg, '=', &value);

        if (!twEqual(arg, setting)) {
            iter_next(args); // eat previous argument
            if (!cmd_project_change(args, &cmd_data, setting, value)) {
                return false;
            }
        } else {
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
    }

    return true;
}

