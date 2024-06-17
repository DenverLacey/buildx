#include "cmd.h"

#include "argiter.h"
#include "conf.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct {
    const char *exe_name;
    Dialect dialect;
    const char *out_dir;
    const char *src_dir;
} PremakeSettings;

static bool read_premake_file(PremakeSettings *pms) {
    bool result = true;
    FILE *f = NULL;

    f = fopen("./premake5.lua", "r");
    if (!f) {
        logprint(LOG_FATAL, "Couldn't open premake file at '%s'.", "./premake5.lua");
        RETURN(false);
    }

    char *line_cstr = NULL;
    size_t linecap = 0;
    ssize_t linelen = 0;

    bool dialect_set = false;
    bool targetdir_done = false;

    while ((linelen = getline(&line_cstr, &linecap, f)) != -1) {
        twString line = (twString){.bytes = line_cstr, .length = linelen - 1};
        line = twTrimLeftUTF8(line);

        // TODO: Fix bug: check for both single and double quote delims
        if (twStartsWith(line, twStatic("project"))) {
            twSplitAnyASCII(line, "'\"", &line);
            twString exe = twSplitUTF8(line, line.bytes[-1], NULL);
            pms->exe_name = twDupToC(exe);
        } else if (twStartsWith(line, twStatic("cdialect")) ||
                   twStartsWith(line, twStatic("cppdialect")))
        {
            dialect_set = true;
            twSplitAnyASCII(line, "'\"", &line);
            const char *dialect = twDupToC(twSplitUTF8(line, line.bytes[-1], NULL));
            pms->dialect = dialect_from_str(dialect);
            free((void*)dialect);
        } else if (twStartsWith(line, twStatic("includedirs"))) {
            line = twDrop(line, sizeof("includedirs") - 1);

            do {
                if (line.length == 0) {
                    linelen = getline(&line_cstr, &linecap, f);
                    line.bytes = line_cstr;
                    line.length = linelen - 1;
                }
                line = twTrimLeftUTF8(line);
            } while (line.length == 0);
            
            if (twFirstUTF8(line) != '{') {
                logprint(LOG_ERROR, "Invalid premake5.lua file: No '{' after 'includedirs'.");
                RETURN(false);
            }

            line = twDrop(line, 1); // eat '{'

            do {
                if (line.length == 0) {
                    linelen = getline(&line_cstr, &linecap, f);
                    line.bytes = line_cstr;
                    line.length = linelen - 1;
                }
                line = twTrimLeftUTF8(line);
            } while (line.length == 0);

            twChar delim = twFirstUTF8(line);
            if (delim != '"' && delim != '\'') {
                logprint(LOG_ERROR, "Invalid premake5.lua file: Expected string literal.");
                RETURN(false);
            }

            line = twDrop(line, 1); // eat delim

            pms->src_dir = twDupToC(twSplitUTF8(line, delim, NULL));
        } else if (twStartsWith(line, twStatic("targetdir")) && !targetdir_done) {
            targetdir_done = true;
            twSplitAnyASCII(line, "'\"", &line);
            twString out_dir = twSplitUTF8(line, line.bytes[-1], NULL);
            out_dir = twSplitUTF8(out_dir, '/', NULL); // WARN: Robustness
            pms->out_dir = twDupToC(out_dir);
        }
    }

    if (!dialect_set || !targetdir_done ||
        pms->exe_name == NULL ||
        pms->out_dir == NULL ||
        pms->src_dir == NULL)
    {
        logprint(LOG_FATAL, "premake file is incomplete.");
        RETURN(false);
    }

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
}

static bool create_conf_from_premake_file(Conf  *conf) {
    static_assert(MAJOR_VERSION == 0 && MINOR_VERSION == 4 && PATCH_VERSION == 2, "Version has changed. Confirm this is still correct.");

    PremakeSettings pms;
    if (!read_premake_file(&pms)) {
        return false;
    }

    char proj_dir_buf[PATH_MAX];
    const char *proj_dir_in_buf = getcwd(proj_dir_buf, sizeof(proj_dir_buf));
    if (!proj_dir_in_buf) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to get CWD: %s.", err);
        return false;
    }

    char *proj_dir = malloc(strlen(proj_dir_in_buf));
    strcpy(proj_dir, proj_dir_in_buf);

    *conf = (Conf) {
        .buildx = (BuildxConf){.major = MAJOR_VERSION, .minor = MINOR_VERSION, .patch = PATCH_VERSION},
        .proj = (ProjConf) {
            .proj_dir = proj_dir,
            .exe_name = pms.exe_name,
            .out_dir = pms.out_dir,
            .src_dir = pms.src_dir,
            .dialect = pms.dialect,
        }
    };

    return true;
}

static bool cmd_project_upgrade(void) {
    const char *build_dir = NULL;
    struct stat s = {0};
    if (stat("./" BUILDX_DIR, &s) == 0) {
        if (!S_ISDIR(s.st_mode)) {
            logprint(
                LOG_FATAL,
                "Non-directory item called '.buildx' found in project. Please rename and run 'bx project upgrade' again."
            );
            return false;
        }
        build_dir = BUILDX_DIR;
    } else if (stat("./build", &s) == 0) {
        if (S_ISDIR(s.st_mode)) {
            build_dir = "build";
        }
    }

    Conf conf = {0};
    bool conf_was_created = false;
    if (build_dir) {
        char conf_path[PATH_MAX];
        snprintf(conf_path, sizeof(conf_path), "./%s/conf.ini", build_dir);

        if (access(conf_path, F_OK) != 0) {
            conf_was_created = true;
            if (!create_conf_from_premake_file(&conf)) {
                return false;
            }
        } else if (!read_conf(conf_path, &conf)) {
            conf_was_created = true;
            if (!create_conf_from_premake_file(&conf)) {
                return false;
            }
        }
    } else {
        conf_was_created = true;
        if (!create_conf_from_premake_file(&conf)) {
            return false;
        }
    }

    if (!conf_was_created &&
        (strcmp(build_dir, BUILDX_DIR) == 0) &&
        version_is_current(conf.buildx.major, conf.buildx.minor, conf.buildx.patch))
    {
        return true;
    }

    // Delete old build directory
    if (build_dir) {
        char cmd[PATH_MAX];
        snprintf(cmd, sizeof(cmd), "rm -rf ./%s", build_dir);

        int c = prompt("Delete old build directory '%s' [y/n]: ", build_dir);
        if (c != 'y' && c != 'Y') {
            logprint(LOG_INFO, "Stopping upgrade because of user response.");
            logprint(LOG_INFO, "If you want to upgrade, either preserve old build directory");
            logprint(LOG_INFO, "and rerun or let upgrade automatically replace it.");
            return false;
        }

        if (system(cmd) == -1) {
            logprint(LOG_ERROR, "Failed to delete old build directory.");
            return false;
        }
    }

    if (mkdir(BUILDX_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create new build directory: %s.", err);
        return false;
    }

    char real_proj_dir[PATH_MAX];
    conf.proj.proj_dir = realpath(".", real_proj_dir);
    if (!conf.proj.proj_dir) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to canonicalize project directory: %s.", err);
        return false;
    }

    return write_conf("./"BUILDX_DIR"/conf.ini", conf.proj);
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

        if (!cmd_project_upgrade()) {
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

