#include "cmd.h"

#include "utils.h"
#include "conf.h"

#include <errno.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_types/_mode_t.h>
#include <sys/stat.h>
#include <sys/syslimits.h>

typedef struct CmdNewData {
    const char *project_path;
    const char *project_name;
    const char *executable_name;
    const char *output_dir;
    const char *src_dir;
    Dialect dialect;
    bool create_project_dir;
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

    new_data->output_dir = path;

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

    new_data->executable_name = name;

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

bool process_options(ArgIter *args, const char *bx_path, CmdNewData *cmd_data) {
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

    cmd_data->create_project_dir = args->length != 0;

    if (args->length != 0) {
        cmd_data->project_path = iter_next(args);
        if (!cmd_data->project_path) {
            logprint(LOG_FATAL, "project_path was null.");
            return false;
        }

        if (strcmp(cmd_data->project_path, ".") == 0) {
            chdir(bx_path);
            char path_buf[PATH_MAX];
            const char *cwd = getcwd(path_buf, PATH_MAX);
            if (!cwd) {
                logprint(LOG_FATAL, "Failed to get cwd.");
                return false;
            }

            char *real_buf = malloc(PATH_MAX); // LEAK
            cmd_data->project_path = realpath(cwd, real_buf);
            if (!cmd_data->project_path) {
                const char *err = strerror(errno);
                logprint(LOG_FATAL, "Failed to canonicalize cwd: %s.", err);
                return false;
            }
        }

    } else {
        chdir(bx_path);
        char path_buf[PATH_MAX];
        const char *cwd = getcwd(path_buf, PATH_MAX);
        if (!cwd) {
            logprint(LOG_FATAL, "Failed to get cwd.");
            return false;
        }

        char *real_buf = malloc(PATH_MAX); // LEAK
        cmd_data->project_path = realpath(cwd, real_buf);
        if (!cmd_data->project_path) {
            const char *err = strerror(errno);
            logprint(LOG_FATAL, "Failed to canonicalize cwd: %s.", err);
            return false;
        }
    }

    cmd_data->project_name = strrchr(cmd_data->project_path, '/');
    if (!cmd_data->project_name) {
        cmd_data->project_name = cmd_data->project_path;
    } else {
        cmd_data->project_name++; // remove leading '/'
    }

    if (!cmd_data->executable_name) {
        cmd_data->executable_name = cmd_data->project_name;
    }

    return true;
}

bool make_project_directories(CmdNewData *cmd_data) {
    if (cmd_data->create_project_dir) {
        int status = mkdir(cmd_data->project_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            const char *err = strerror(errno);
            logprint(LOG_ERROR, "Failed to create project directory: %s.", err);
            return false;
        }
    }

    int status;
    char path[PATH_MAX];
    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin";
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
            logprint(LOG_WARN, "'%s' already exists.", cmd_data->project_path);
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
            logprint(LOG_WARN, "'%s' already exists.", cmd_data->project_path);
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
    Proj_Conf conf = {
        .proj_dir = cmd_data->project_path ? cmd_data->project_path : ".",
        .exe_name = cmd_data->executable_name,
        .out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin",
        .src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src",
        .dialect = cmd_data->dialect,
    };

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s/conf.ini", conf.proj_dir, BUILDX_DIR);

    return write_conf(path, conf);
}

bool make_main_file(CmdNewData *cmd_data) {
    bool result = true;
    FILE *f = NULL;

    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
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
    fprintf(f, "    printf(\"Hello, %s!\\n\");\n", cmd_data->executable_name);
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

    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin";
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

    fprintf(f, "workspace '%s'\n", cmd_data->project_name);
    fprintf(f, "configurations { 'debug', 'release' }\n");
    fprintf(f, "\n");
    fprintf(f, "project '%s'\n", cmd_data->executable_name);
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
    fclose(f);
    return result;
}

bool make_build_scripts(CmdNewData *cmd_data) {
    bool result = true;
    FILE *build_debug = NULL;
    FILE *build_release = NULL;
    FILE *run_debug = NULL;
    FILE *run_release = NULL;

    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin";
    const char *backend = "gmake2";

    mode_t mode = S_IRUSR | S_IWUSR | S_IXUSR;

    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s/build_debug.sh", proj_dir, BUILDX_DIR);

    build_debug = fopen(path, "wx");
    if (!build_debug) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    fprintf(build_debug, "#!/bin/bash\n");
    fprintf(build_debug, "\n");
    fprintf(build_debug, "# premake commands\n");
    fprintf(build_debug, "premake5 %s\n", backend);
    fprintf(build_debug, "premake5 export-compile-commands\n");
    fprintf(build_debug, "cp compile_commands/debug.json compile_commands.json\n");
    fprintf(build_debug, "mv compile_commands %s/compile_commands\n", BUILDX_DIR);
    fprintf(build_debug, "\n");
    fprintf(build_debug, "# backend commands\n"); // TODO: Hardcoded backend.
    fprintf(build_debug, "make config=debug\n");
    fprintf(build_debug, "\n");

    int chmod_result = chmod(path, mode);
    if (chmod_result == -1) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to give permissions to 'build_debug.sh': %s", err);
        RETURN(false);
    }

    snprintf(path, sizeof(path), "%s/%s/build_release.sh", proj_dir, BUILDX_DIR);

    build_release = fopen(path, "wx");
    if (!build_release) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    fprintf(build_release, "#!/bin/bash\n");
    fprintf(build_release, "\n");
    fprintf(build_release, "# premake commands\n");
    fprintf(build_release, "premake5 --file=%s/premake5.lua %s\n", BUILDX_DIR, backend);
    fprintf(build_release, "premake5 --file=%s/premake5.lua export-compile-commands\n", BUILDX_DIR);
    fprintf(build_release, "cp %s/compile_commands/debug.json compile_commands.json\n", BUILDX_DIR);
    fprintf(build_release, "\n");
    fprintf(build_release, "# backend commands\n"); // TODO: Hardcoded backend.
    fprintf(build_release, "make config=release\n");
    fprintf(build_release, "\n");

    chmod_result = chmod(path, mode);
    if (chmod_result == -1) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to give permissions to 'build_release.sh': %s", err);
        RETURN(false);
    }

    snprintf(path, sizeof(path), "%s/%s/run_debug.sh", proj_dir, BUILDX_DIR);

    run_debug = fopen(path, "wx");
    if (!run_debug) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    // TODO: Forwarding arguments to the executable
    fprintf(run_debug, "#!/bin/bash\n");
    fprintf(run_debug, "\n");
    fprintf(run_debug, "if ! [ -f %s/debug/%s ]; then\n", out_dir, cmd_data->executable_name);
    fprintf(run_debug, "    echo \"ERROR: No executable. use `bx build --debug` first.\"\n");
    fprintf(run_debug, "    exit\n");
    fprintf(run_debug, "fi\n");
    fprintf(run_debug, "\n");
    fprintf(run_debug, "%s/debug/%s\n", out_dir, cmd_data->executable_name);
    fprintf(run_debug, "\n");

    chmod_result = chmod(path, mode);
    if (chmod_result == -1) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to give permissions to 'run_debug.sh': %s", err);
        RETURN(false);
    }

    snprintf(path, sizeof(path), "%s/%s/run_release.sh", proj_dir, BUILDX_DIR);

    run_release = fopen(path, "wx");
    if (!run_release) {
        if (errno == EEXIST) {
            logprint(LOG_WARN, "'%s' already exists.", path);
            RETURN(true);
        }
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to create '%s': %s", path, err);
        RETURN(false);
    }

    // TODO: Forwarding arguments to the executable
    fprintf(run_release, "#!/bin/bash\n");
    fprintf(run_release, "\n");
    fprintf(run_release, "if ! [ -f %s/release/%s ]; then\n", out_dir, cmd_data->executable_name);
    fprintf(run_release, "    echo \"ERROR: No executable. use `bx build --release` first.\"\n");
    fprintf(run_release, "    exit\n");
    fprintf(run_release, "fi\n");
    fprintf(run_release, "\n");
    fprintf(run_release, "%s/release/%s\n", out_dir, cmd_data->executable_name);
    fprintf(run_release, "\n");

    chmod_result = chmod(path, mode);
    if (chmod_result == -1) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to give permissions to 'run_release.sh': %s", err);
        RETURN(false);
    }

CLEAN_UP_AND_RETURN:
    if (build_debug) fclose(build_debug);
    if (build_release) fclose(build_release);
    if (run_debug) fclose(run_debug);
    if (run_release) fclose(run_release);
    return result;
}

bool cmd_new(ArgIter *args, const char *bx_path) {
    bool ok;
    CmdNewData cmd_data = {0};
    
    ok = process_options(args, bx_path, &cmd_data);
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

    ok = make_build_scripts(&cmd_data);
    if (!ok) {
        return false;
    }

    return true;
}

/* TODO:
*  What should be done in the case where the project directory already exists?
*  Is this an edge case we want to support?
*  I could see wanting to run `new` command more than once in order to update
*  things, but perhaps that should be its own command.
*/

/* TODO:
*  Use the new command to set the backend: make, vsproj, xcode, etc.
*/

