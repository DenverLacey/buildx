#include "argiter.h"
#include "cmd.h"
#include "conf.h"
#include "utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslimits.h>
#include <sys/unistd.h>
#include <unistd.h>

typedef struct CmdInstallData {
    const char *dest;
    const char *name;
    bool debug;
} CmdInstallData;

void usage_install(void) {
    printf("Usage: bx install [-h] [-D DESTINATION] [-n NAME] [-d]\n");
    printf("Options:\n");
    printf("    -h, --help:        Show this help message.\n");
    printf("    -D, --destination: Specify destination of install. Default is '/usr/local/bin'.\n");
    printf("    -n, --name:        Override name of executable.\n");
    printf("    -d, --debug:       Install debug executable. Adds '-debug' suffix to name.\n");
}

static bool cmd_install_help(ArgIter *args, void *cmd_data) {
    UNUSED(args, cmd_data);
    usage_install();
    exit(0);
    return true;
}

static bool cmd_install_destination(ArgIter *args, void *cmd_data) {
    CmdInstallData *data = (CmdInstallData*)cmd_data;

    const char *dest = iter_next(args);
    if (!dest) {
        logprint(LOG_ERROR, "Expected a path after `-D/--destination` flag.");
        return false;
    }

    logprint(LOG_DEBUG, "dest = '%s'", dest);

    data->dest = dest;
    return true;
}

static bool cmd_install_name(ArgIter *args, void *cmd_data) {
    CmdInstallData *data = (CmdInstallData*)cmd_data;

    const char *name = iter_next(args);
    if (!name) {
        logprint(LOG_ERROR, "Expected an executable name after `-n/--name` flag.");
        return false;
    }

    data->name = name;
    return true;
}

static bool cmd_install_debug(ArgIter *args, void *cmd_data) {
    UNUSED(args);
    CmdInstallData *data = (CmdInstallData*)cmd_data;
    data->debug = true;
    return true;
}

static const CmdFlagInfo flags[] = {
    (CmdFlagInfo){
        .short_name = "h",
        .long_name = "help",
        .cmd = cmd_install_help
    },
    (CmdFlagInfo){
        .short_name = "D",
        .long_name = "destination",
        .cmd = cmd_install_destination
    },
    (CmdFlagInfo){
        .short_name = "n",
        .long_name = "name",
        .cmd = cmd_install_name
    },
    (CmdFlagInfo){
        .short_name = "d",
        .long_name = "debug",
        .cmd = cmd_install_debug
    },
};

static const size_t flags_length = sizeof(flags) / sizeof(flags[0]);

bool cmd_install(ArgIter *args) {
    CmdInstallData cmd_data = {0};
    
    if (!process_options(args, &cmd_data, flags, flags_length)) {
        return false;
    }

    Conf conf;
    if (!read_conf(BUILDX_DIR"/conf.ini", &conf)) {
        return false;
    }

    const char *dest = cmd_data.dest ? cmd_data.dest : "/usr/local/bin";

    bool name_overriden = false;
    const char *name;
    if (!cmd_data.name) {
        name = conf.proj.exe_name;
    } else {
        name_overriden = true;
        name = cmd_data.name;
    }

    const char *debug_suffix = "";
    if (cmd_data.debug && !name_overriden) {
        debug_suffix = "-debug";
    }

    char exe_dir[PATH_MAX];
    snprintf(
        exe_dir, sizeof(exe_dir),
        "%s/%s/%s/%s",
        conf.proj.proj_dir, conf.proj.out_dir, cmd_data.debug ? "debug" : "release", conf.proj.exe_name
    );

    if (access(exe_dir, F_OK) != 0) {
        logprint(LOG_ERROR, "No %s executable to install. Please use the `bx build --%s` command and try again.",
             cmd_data.debug ? "debug" : "release",
             cmd_data.debug ? "debug" : "release"
        );
        return false;
    }

    char link_dir[PATH_MAX];
    snprintf(link_dir, sizeof(link_dir),"%s/%s%s", dest, name, debug_suffix);

    logprint(LOG_DEBUG, "link_dir = '%s'", link_dir);

    if (access(link_dir, F_OK) == 0) {
        if (unlink(link_dir) != 0) {
            const char *err = strerror(errno);
            logprint(LOG_FATAL, "Failed to delete previous install: %s.", err);
            return false;
        }
    }

    if (link(exe_dir, link_dir) != 0) {
        const char *err = strerror(errno);
        logprint(LOG_FATAL, "Failed to create install: %s.", err);
        return false;
    }

    return true;
}

