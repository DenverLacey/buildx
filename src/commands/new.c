#include "new.h"
#include "argiter.h"
#include "cmd.h"
#include "errors.h"
#include "utils.h"
#include <stdio.h>

static ResultCode cmd_new_help(ArgIter *args) {
    UNUSED(args);

    printf("USAGE: bx new [-h] [-o OUTPUT_DIR] [-s SRC_DIR] [-d {dialects...}] [--cpp] [-n NAME] [project_name]\n");
    printf("Positional Arguments:\n");
    printf("    project_name:     Name of the project (and the output executable if `-n` not passed.)\n");
    printf("Options:\n");
    printf("    -h, --help:       Show this help message.\n");
    printf("    -o, --output_dir: Set the output directory. Default is `bin`.\n");
    printf("    -s, --src_dir:    Set the source directory. Default is `src`.\n");
    printf("    -d, --dialect:    Set the language standard. Default is `c11`.\n");
    printf("        Variants: {c99,c11,c17,c++11,c++14,c++17}\n");
    printf("    --cpp:            Make a C++ project. Alias for `-d c++17`.\n");
    printf("    -n, --name:       Override name of executable. If not set, will be the same as `project_name`.\n");

    return OK;
}

static ResultCode cmd_new_cpp(ArgIter *args) {
    UNUSED(args);

    printf("[INFO] `--cpp` flag passed.\n");

    return OK;
}

const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_new_help
    },
    (CmdFlagInfo){
        .short_name = "",
        .long_name = "cpp",
        .cmd = cmd_new_cpp
    }
};

const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

ResultCode cmd_new(ArgIter *args) {
    while (args->length != 0) {
        for (size_t i = 0; i < flags_length; i++) {
            const CmdFlagInfo *flag = &flags[i];
            if (iter_check_flags(args, flag->names)) {
                ResultCode result = flag->cmd(args);
                if (result != OK) {
                    return result;
                }
            }
        }
        iter_next(args);
    }

    // TODO: Actually make a new project folder.

    return OK;
}

