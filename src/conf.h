#ifndef _CONF_H_
#define _CONF_H_

#include "utils.h"

#include <stdbool.h>

typedef struct {
	int major;
	int minor;
	int patch;
} Buildx_Conf;

typedef struct {
	const char *proj_dir;
	const char *exe_name;
	const char *out_dir;
	const char *src_dir;
	Dialect dialect;
} Proj_Conf;

typedef struct {
	Buildx_Conf buildx;
	Proj_Conf proj;
} Conf;

bool write_conf(const char *path, Proj_Conf conf);
bool read_conf(const char *path, Conf *conf);

#endif // _CONF_H_ 

