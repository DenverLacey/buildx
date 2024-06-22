#include "cmd.h"

#include "utils.h"
#include "conf.h"

#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

typedef struct CmdNewData {
    const char *proj_path;
    const char *proj_name;
    const char *exe_name;
    const char *out_dir;
    const char *src_dir;
    Dialect dialect;
    bool create_proj_dir;
} CmdNewData;

static void usage_new(void) {
    printf("Usage: bx new [-h] [-o OUTPUT_DIR] [-s SRC_DIR] [-d {dialects...}] [--cpp] [-n NAME] [project_name]\n");
    printf("Positional Arguments:\n");
    printf("    project_name:     Name of the project (and the output executable if `-n` not passed.)\n");
    printf("Options:\n");
    printf("    -h, --help:       Show this help message.\n");
    printf("    -o, --output_dir: Set the output directory. Default is `bin`.\n");
    printf("    -s, --src_dir:    Set the source directory. Default is `src`.\n");
    printf("    -d, --dialect:    Set the language standard. Default is `c99`.\n");
    printf("        Variants: {c99,c11,c17,c++11,c++14,c++17}\n");
    printf("    --cpp:            Make a C++ project. Alias for `-d c++17`.\n");
    printf("    -n, --name:       Override name of executable. If not set, will be the same as `project_name`.\n");
}

static bool cmd_new_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_new();
    exit(0);
    return true;
}

static bool cmd_new_output_dir(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *path = iter_next(args);
    if (!path) {
        logprint(LOG_ERROR, "Expected a path after `-o/--output_dir` flag.");
        return false;
    }

    new_data->out_dir = path;

    return true;
}

static bool cmd_new_src_dir(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *path = iter_next(args);
    if (!path) {
        logprint(LOG_ERROR, "Expected a path after `-s/--src_dir` flag.");
        return false;
    }

    new_data->src_dir = path;

    return true;
}

static bool cmd_new_dialect(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *dialect = iter_next(args);
    if (!dialect) {
        logprint(LOG_ERROR, "Expected a dialect after `-d/--dialect` flag.");
        return false;
    }

    bool found = false;
    for (Dialect d = C99; d < DIALECT_COUNT; d++) {
        if (strcmp(dialect, dialect_names[d]) == 0) {
            new_data->dialect = d;
            found = true;
            break;
        }
    }

    if (!found) {
        logprint(LOG_ERROR, "'%s' is not a valid dialect variant.", dialect);
        return false;
    }

    return true;
}

static bool cmd_new_cpp(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdNewData *new_data = (CmdNewData *)cmd_data;
    new_data->dialect = CPP17;

    return true;
}

static bool cmd_new_name(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *name = iter_next(args);
    if (!name) {
        logprint(LOG_ERROR, "Expected an identifier after `-n/--name` flag.");
        return false;
    }

    new_data->exe_name = name;

    return true;
}

static const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_new_help
    },
    (CmdFlagInfo){
        .short_name = "o",
        .long_name = "output_dir",
        .cmd = cmd_new_output_dir
    },
    (CmdFlagInfo){
        .short_name = "s",
        .long_name = "src_dir",
        .cmd = cmd_new_src_dir
    },
    (CmdFlagInfo){
        .short_name = "d",
        .long_name = "dialect",
        .cmd = cmd_new_dialect
    },
    (CmdFlagInfo){
        .short_name = "",
        .long_name = "cpp",
        .cmd = cmd_new_cpp
    },
    (CmdFlagInfo){
        .short_name = "n",
        .long_name = "name",
        .cmd = cmd_new_name
    }
};

static const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

static bool process_new_options(ArgIter *args, CmdNewData *cmd_data) {
#if 0
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                bool ok = flag->cmd(args, cmd_data);
                if (!ok) {
                    usage_new();
                    return false;
                }
            }
        }

        if (!flag_found) {
            break;
        }
    }
#endif

    if (!process_options(args, cmd_data, flags, flags_length)) {
        usage_new();
        return false;
    }

    cmd_data->create_proj_dir = args->length != 0;

    if (args->length != 0) {
        cmd_data->proj_path = iter_next(args);
        if (!cmd_data->proj_path) {
            logprint(LOG_FATAL, "project_path was null.");
            return false;
        }

        if (strcmp(cmd_data->proj_path, ".") == 0) {
            char *proj_path = malloc(PATH_MAX);
            cmd_data->proj_path = getcwd(proj_path, PATH_MAX);
            if (!cmd_data->proj_path) {
                logprint(LOG_FATAL, "Failed to get cwd.");
                return false;
            }
        } else {
            char cwd[PATH_MAX];
            if (!getcwd(cwd, PATH_MAX)) {
                logprint(LOG_FATAL, "Failed to get cwd.");
                return false;
            }

            char *proj_path = malloc(PATH_MAX);
            snprintf(proj_path, PATH_MAX, "%s/%s", cwd, cmd_data->proj_path);

            cmd_data->proj_path = proj_path;
        }
    } else {
        char *proj_path = malloc(PATH_MAX);
        cmd_data->proj_path = getcwd(proj_path, PATH_MAX);
        if (!cmd_data->proj_path) {
            logprint(LOG_FATAL, "Failed to get cwd.");
            return false;
        }
    }

    cmd_data->proj_name = strrchr(cmd_data->proj_path, '/');
    if (!cmd_data->proj_name) {
        cmd_data->proj_name = cmd_data->proj_path;
    } else {
        cmd_data->proj_name++; // remove leading '/'
    }

    if (!cmd_data->exe_name) {
        cmd_data->exe_name = cmd_data->proj_name;
    }

    return true;
}

bool make_project_directories(CmdNewData *cmd_data) {
    if (cmd_data->create_proj_dir) {
        int status = mkdir(cmd_data->proj_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            const char *err = strerror(errno);
            logprint(LOG_ERROR, "Failed to create project directory: %s.", err);
            return false;
        }
    }

    int status;
    char path[PATH_MAX];
    const char *proj_dir = cmd_data->proj_path ? cmd_data->proj_path : ".";
    const char *out_dir = cmd_data->out_dir ? cmd_data->out_dir : "bin";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";

    // Output Directory
    snprintf(path, sizeof(path), "%s/%s", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create output directory: %s", err);
        return false;
    }

    snprintf(path, sizeof(path), "%s/%s/debug", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        if (status == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", cmd_data->proj_path);
            return true;
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create debug output directory: %s", err);
        return false;
    }

    snprintf(path, sizeof(path), "%s/%s/release", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create release output directory: %s", err);
        return false;
    }

    // Source Directory
    snprintf(path, sizeof(path), "%s/%s", proj_dir, src_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        if (status == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", cmd_data->proj_path);
            return true;
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create source directory: %s", err);
        return false;
    }

    // buildx Directory
    snprintf(path, sizeof(path), "%s/"BUILDX_DIR, proj_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create buildx directory: %s", err);
        return false;
    }

    return true;
}

bool make_config_file(CmdNewData *cmd_data) {
    ProjConf conf = {
        .exe_name = cmd_data->exe_name,
        .out_dir = cmd_data->out_dir ? cmd_data->out_dir : "bin",
        .src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src",
        .dialect = cmd_data->dialect,
    };

    if (cmd_data->proj_path) {
        conf.proj_dir = cmd_data->proj_path;
    } else {
        char cwd[PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) {
            const char *err = strerror(errno);
            logprint(LOG_FATAL, "Failed to get CWD: %s.", err);
        }
        conf.proj_dir = cwd;
    }

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s/conf.ini", conf.proj_dir, BUILDX_DIR);

    return write_conf(path, conf);
}

bool make_main_file(CmdNewData *cmd_data) {
    bool result = true;
    FILE *f = NULL;

    const char *proj_dir = cmd_data->proj_path ? cmd_data->proj_path : ".";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";
    const char *ext = cmd_data->dialect < CPP11 ? "c" : "cpp";

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s/main.%s", proj_dir, src_dir, ext);

    f = fopen(path, "wx");
    if (!f) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "\n");
    fprintf(f, "int main(void) {\n");
    fprintf(f, "    printf(\"Hello, %s!\\n\");\n", cmd_data->exe_name);
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
}

bool make_premake_file(CmdNewData *cmd_data) {
    bool result = true;
    FILE *f = NULL;

    const char *proj_dir = cmd_data->proj_path ? cmd_data->proj_path : ".";
    const char *out_dir = cmd_data->out_dir ? cmd_data->out_dir : "bin";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";

    const char *lang = cmd_data->dialect < CPP11 ? "C" : "C++";
    
    const char *dialect_lang = cmd_data->dialect < CPP11 ? "c" : "cpp";

    char dialect[8];
    strcpy(dialect, dialect_names[cmd_data->dialect]);
    strupper(dialect);

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/premake5.lua", proj_dir);

    f = fopen(path, "wx");
    if (!f) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    fprintf(f, "workspace '%s'\n", cmd_data->proj_name);
    fprintf(f, "configurations { 'debug', 'release' }\n");
    fprintf(f, "\n");
    fprintf(f, "project '%s'\n", cmd_data->exe_name);
    fprintf(f, "    kind 'ConsoleApp'\n");
    fprintf(f, "    language '%s'\n", lang);
    fprintf(f, "    %sdialect '%s'\n", dialect_lang, dialect);
    fprintf(f, "\n");
    fprintf(f, "    files {\n");
    fprintf(f, "        '%s/**.h',\n", src_dir);
    fprintf(f, "        '%s/**.c',\n", src_dir);
    fprintf(f, "        '%s/**.hpp',\n", src_dir);
    fprintf(f, "        '%s/**.cpp',\n", src_dir);
    fprintf(f, "        '%s/**.hxx',\n", src_dir);
    fprintf(f, "        '%s/**.cxx',\n", src_dir);
    fprintf(f, "        '%s/**.cc',\n", src_dir);
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "    includedirs {\n");
    fprintf(f, "        '%s'\n", src_dir);
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "    filter 'action:gmake2'\n");
    fprintf(f, "        buildoptions {\n");
    fprintf(f, "            '-Wpedantic',\n");
    fprintf(f, "            '-Wall',\n");
    fprintf(f, "            '-Wextra',\n");
    fprintf(f, "            '-Werror',\n");
    fprintf(f, "        }\n");
    fprintf(f, "\n");
    fprintf(f, "    filter 'configurations:debug'\n");
    fprintf(f, "        defines { 'DEBUG' }\n");
    fprintf(f, "        targetdir '%s/debug'\n", out_dir);
    fprintf(f, "        symbols 'On'\n");
    fprintf(f, "        optimize 'Debug'\n");
    fprintf(f, "\n");
    fprintf(f, "    filter 'configurations:release'\n");
    fprintf(f, "        defines { 'NDEBUG' }\n");
    fprintf(f, "        targetdir '%s/release'\n", out_dir);
    fprintf(f, "        optimize 'Full'\n");
    fprintf(f, "\n");

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
}

bool cmd_new(ArgIter *args) {
    bool ok;
    CmdNewData cmd_data = {0};
    
    ok = process_new_options(args, &cmd_data);
    if (!ok) {
        return false;
    }

    ok = make_project_directories(&cmd_data);
    if (!ok) {
        return false;
    }

    ok = make_config_file(&cmd_data);
    if (!ok) {
        return false;
    }

    ok = make_main_file(&cmd_data);
    if (!ok) {
        return false;
    }

    ok = make_premake_file(&cmd_data);
    if (!ok) {
        return false;
    }

    return true;
}

