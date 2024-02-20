#include "new.h"
#include "argiter.h"
#include "cmd.h"
#include "errors.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

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
        printf("ERROR: Expected a path after `-o/--output_dir` flag.\n");
        return NO_ARG_VALUE;
    }

    new_data->output_dir = path;

    return OK;
}

static ResultCode cmd_new_src_dir(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *path = iter_next(args);
    if (!path) {
        printf("ERROR: Expected a path after `-s/--src_dir` flag.\n");
        return NO_ARG_VALUE;
    }

    new_data->src_dir = path;

    return OK;
}

static ResultCode cmd_new_dialect(ArgIter *args, void *cmd_data) {
    CmdNewData *new_data = (CmdNewData *)cmd_data;

    const char *dialect = iter_next(args);
    if (!dialect) {
        printf("ERROR: Expected a dialect after `-d/--dialect` flag.\n");
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
        printf("ERROR: '%s' is not a valid dialect variant.\n", dialect);
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
        printf("ERROR: Expected an identifier after `-n/--name` flag.\n");
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

ResultCode cmd_new(ArgIter *args) {
    CmdNewData cmd_data = {0};

    // Process options
    while (args->length != 0) {
        bool flag_found = false;

        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                iter_next(args);
                flag_found = true;
                ResultCode result = flag->cmd(args, &cmd_data);
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
        cmd_data.project_name = iter_next(args);
        if (!cmd_data.executable_name) {
            cmd_data.executable_name = cmd_data.project_name;
        }
    }

    // TODO: Actually make a new project folder.

    return OK;
}

