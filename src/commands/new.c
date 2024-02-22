#include "new.h"
#include "argiter.h"
#include "cmd.h"
#include "errors.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>

typedef enum Dialect {
    C99 = 0,
    C11,
    C17,
    CPP11,
    CPP14,
    CPP17,

    DIALECT_COUNT
} Dialect;

const char *dialect_names[] = {
    "c99",
    "c11",
    "c17",
    "c++11",
    "c++14",
    "c++17"
};

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
    printf("USAGE: bx new [-h] [-o OUTPUT_DIR] [-s SRC_DIR] [-d {dialects...}] [--cpp] [-n NAME] [project_name]\n");
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

static ResultCode cmd_new_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_new();
    return OK;
}

static ResultCode cmd_new_output_dir(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *path = iter_next(args);
    if (!path) {
        logprint(ERROR, "Expected a path after `-o/--output_dir` flag.");
        return NO_ARG_VALUE;
    }

    new_data->output_dir = path;

    return OK;
}

static ResultCode cmd_new_src_dir(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *path = iter_next(args);
    if (!path) {
        logprint(ERROR, "Expected a path after `-s/--src_dir` flag.");
        return NO_ARG_VALUE;
    }

    new_data->src_dir = path;

    return OK;
}

static ResultCode cmd_new_dialect(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *dialect = iter_next(args);
    if (!dialect) {
        logprint(ERROR, "Expected a dialect after `-d/--dialect` flag.");
        return NO_ARG_VALUE;
    }

    bool not_found = true;
    for (Dialect d = C99; d < DIALECT_COUNT; d++) {
        if (strcmp(dialect, dialect_names[d]) == 0) {
            new_data->dialect = d;
            not_found = false;
            break;
        }
    }

    if (not_found) {
        logprint(ERROR, "'%s' is not a valid dialect variant.", dialect);
        return BAD_ARG_VALUE;
    }

    return OK;
}

static ResultCode cmd_new_cpp(ArgIter *args, void *cmd_data) {
    UNUSED(args);

    CmdNewData *new_data = (CmdNewData *)cmd_data;
    new_data->dialect = CPP17;

    return OK;
}

static ResultCode cmd_new_name(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *name = iter_next(args);
    if (!name) {
        logprint(ERROR, "Expected an identifier after `-n/--name` flag.");
        return NO_ARG_VALUE;
    }

    new_data->executable_name = name;

    return OK;
}

const CmdFlagInfo flags[] = {
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

const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

ResultCode process_options(ArgIter *args, CmdNewData *cmd_data) {
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                ResultCode result = flag->cmd(args, cmd_data);
                if (result != OK) {
                    usage_new();
                    return result;
                }
            }
        }

        if (!flag_found) {
            break;
        }
    }

    if (args->length != 0) {
        // TODO: Handle the user explicitly passed `.` as the project path
        cmd_data->project_path = iter_next(args);

        cmd_data->project_name = strrchr(cmd_data->project_path, '/');
        if (!cmd_data->project_name) {
            cmd_data->project_name = cmd_data->project_path;
        } else {
            cmd_data->project_name++; // remove leading '/'
        }

        if (!cmd_data->executable_name) {
            cmd_data->executable_name = cmd_data->project_name;
        }
    }

    return OK;
}

ResultCode make_project_directories(CmdNewData *cmd_data) {
    if (cmd_data->project_path) {
        int status = mkdir(cmd_data->project_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if (status == -1) {
            const char *err = strerror(errno);
            logprint(ERROR, "Failed to create project directory: %s.", err);
            return FAIL_CREATE_DIRECTORY;
        }
    }

    int status;
    char path[1024];
    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";

    snprintf(path, sizeof(path), "%s/%s", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create output directory: %s", err);
        return FAIL_CREATE_DIRECTORY;
    }

    snprintf(path, sizeof(path), "%s/%s/debug", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        if (status == EEXIST) {
            logprint(WARN, "'%s' already exists.", cmd_data->project_path);
            return OK;
        }
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create debug output directory: %s", err);
        return FAIL_CREATE_DIRECTORY;
    }

    snprintf(path, sizeof(path), "%s/%s/release", proj_dir, out_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create release output directory: %s", err);
        return FAIL_CREATE_DIRECTORY;
    }

    snprintf(path, sizeof(path), "%s/%s", proj_dir, src_dir);
    status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (status == -1) {
        if (status == EEXIST) {
            logprint(WARN, "'%s' already exists.", cmd_data->project_path);
            return OK;
        }
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create source directory: %s", err);
        return FAIL_CREATE_DIRECTORY;
    }

    return OK;
}

ResultCode make_main_file(CmdNewData *cmd_data) {
    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";
    const char *ext = cmd_data->dialect < CPP11 ? "c" : "cpp";

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s/main.%s", proj_dir, src_dir, ext);

    FILE *f = fopen(path, "wx");
    if (!f) {
        if (errno == EEXIST) {
            logprint(WARN, "'%s' already exists.", path);
            return OK;
        }
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create '%s': %s", path, err);
        return FAIL_OPEN_FILE;
    }

    fprintf(f, "#include <stdio.h>\n");
    fprintf(f, "\n");
    fprintf(f, "int main(int argc, const char **argv) {\n");
    fprintf(f, "    printf(\"Hello, %s!\\n\");\n", cmd_data->executable_name);
    fprintf(f, "    return 0;\n");
    fprintf(f, "}\n");
    fprintf(f, "\n");

    return OK;
}

ResultCode make_premake_file(CmdNewData *cmd_data) {
    const char *proj_dir = cmd_data->project_path ? cmd_data->project_path : ".";
    const char *out_dir = cmd_data->output_dir ? cmd_data->output_dir : "bin";
    const char *src_dir = cmd_data->src_dir ? cmd_data->src_dir : "src";

    const char *lang = cmd_data->dialect < CPP11 ? "C" : "C++";
    
    const char *dialect_lang = cmd_data->dialect < CPP11 ? "c" : "cpp";

    char dialect[8];
    strcpy(dialect, dialect_names[cmd_data->dialect]);
    strupper(dialect);

    char path[1024];
    snprintf(path, sizeof(path), "%s/premake5.lua", proj_dir);

    FILE *f = fopen(path, "wx");
    if (!f) {
        if (errno == EEXIST) {
            logprint(WARN, "'%s' already exists.", path);
            return OK;
        }
        const char *err = strerror(errno);
        logprint(ERROR, "Failed to create '%s': %s", path, err);
        return FAIL_OPEN_FILE;
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

    return OK;
}

ResultCode cmd_new(ArgIter *args) {
    ResultCode result;
    CmdNewData cmd_data = {0};
    
    result = process_options(args, &cmd_data);
    if (result != OK) {
        return result;
    }

    result = make_project_directories(&cmd_data);
    if (result != OK) {
        return result;
    }

    result = make_main_file(&cmd_data);
    if (result != OK) {
        return result;
    }

    result = make_premake_file(&cmd_data);
    if (result != OK) {
        return result;
    }

    return OK;
}

/* TODO:
*  What should be done in the case where the project directory already exists?
*  Is this an edge case we want to support?
*  I could see wanting to run `new` command more than once in order to update
*  things, but perhaps that should be its own command.
*/

