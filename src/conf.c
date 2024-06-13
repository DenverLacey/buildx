#include "conf.h"
#include "utils.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslimits.h>

bool write_conf(const char *path, Proj_Conf conf) {
    bool result = true;
    FILE *f = NULL;

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

    fprintf(f, "[buildx]\n");
    fprintf(f, "version = %d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);
    fprintf(f, "\n");
    fprintf(f, "[project]\n");
    fprintf(f, "project_directory = %s\n", conf.proj_dir);
    fprintf(f, "executable = %s\n", conf.exe_name);
    fprintf(f, "output_directory = %s\n", conf.out_dir);
    fprintf(f, "source_directory = %s\n", conf.src_dir);
    fprintf(f, "dialect = %s\n", dialect_names[conf.dialect]);

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
}

typedef enum {
    SEC_OPEN,
    SEC_BULIDX,
    SEC_PROJECT,
} Section;

#define SCAN_FIELD(_field_) do {                                      \
    int nscanned = sscanf(line, _field_ " = %s\n", field);            \
    if (nscanned != 1) {                                              \
        logprint(LOG_ERROR, "Unable to parse " _field_ ". %s", line); \
        return false;                                                 \
    }                                                                 \
} while (0)

#define PARSE_PATH_FIELD(_field_, _dst_) do {                         \
    SCAN_FIELD(_field_);                                              \
    conf->proj._dst_ = strdup(field);                                 \
} while (0)

static Conf unset_conf = {
    .buildx.major = -1,
    .buildx.minor = -1,
    .buildx.patch = -1,
    .proj.dialect = -1,
};

bool read_conf(const char *path, Conf *conf) {
    *conf = unset_conf;

    bool result = true;
    FILE *f = NULL;

    char *line = NULL;
    size_t linecap = 0;
    ssize_t len = 0;

    f = fopen(path, "r");
    if (!f) {
        const char *err = strerror(errno);
        logprint(LOG_ERROR, "Failed to open '%s': %s", path, err);
        RETURN(false);
    }

    Section current_section = SEC_OPEN;
    while ((len = getline(&line, &linecap, f)) != -1) {
        if (len == 1) { // empty line
            current_section = SEC_OPEN;
            continue;
        }

        switch (current_section) {
            case SEC_OPEN: {
                if (strcmp(line, "[buildx]\n") == 0) {
                    current_section = SEC_BULIDX;
                } else if (strcmp(line, "[project]\n") == 0) {
                    current_section = SEC_PROJECT;
                } else {
                    logprint(LOG_ERROR, "Unexpected line in conf.ini file: %s\n", line);
                    RETURN(false);
                }
            } break;
            case SEC_BULIDX: {
                if (starts_with(line, "version")) {
                    int nscanned = sscanf(line, "version = %d.%d.%d\n", &conf->buildx.major, &conf->buildx.minor, &conf->buildx.patch);
                    if (nscanned != 3) {
                        logprint(LOG_ERROR, "Unable to parse version. %s", line);
                        RETURN(false);
                    }
                } else {
                    logprint(LOG_ERROR, "Unexpected line in [buildx] section of conf.ini file: %s\n", line);
                    RETURN(false);
                }
            } break;
            case SEC_PROJECT: {
                // TODO: Properly parse out the value.
                // This doesn't account for quoted values.
                char field[PATH_MAX]; 
                if (starts_with(line, "project_directory")) {
                    PARSE_PATH_FIELD("project_directory", proj_dir);
                } else if (starts_with(line, "executable")) {
                    PARSE_PATH_FIELD("executable", exe_name);
                } else if (starts_with(line, "output_directory")) {
                    PARSE_PATH_FIELD("output_directory", out_dir);
                } else if (starts_with(line, "source_directory")) {
                    PARSE_PATH_FIELD("source_directory", src_dir);
                } else if (starts_with(line, "dialect")) {
                    SCAN_FIELD("dialect");
                    conf->proj.dialect = dialect_from_str(field);
                } else {
                    logprint(LOG_ERROR, "Unexpected line in [project] section of conf.ini file: %s\n", line);
                    RETURN(false);
                }
            } break;
            default:
                logprint(LOG_FATAL, "Invalid Section value: %d\n", current_section);
                RETURN(false);
        }
    }

    if (memcmp(conf, &unset_conf, sizeof(Conf)) == 0) {
        logprint(LOG_ERROR, "Configuration incomplete.");
        RETURN(false);
    }

CLEAN_UP_AND_RETURN:
    if (f) fclose(f);
    return result;
}

