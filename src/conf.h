#ifndef _CONF_H_
#define _CONF_H_

#include "utils.h"

#include <stdbool.h>

typedef struct {
	int major;
	int minor;
	int patch;
} BuildxConf;

typedef struct {
	const char *proj_dir;
	const char *exe_name;
	const char *out_dir;
	const char *src_dir;
	Dialect dialect;
} ProjConf;

typedef struct {
	BuildxConf buildx;
	ProjConf proj;
} Conf;

bool write_conf(const char *path, ProjConf conf);
bool read_conf(const char *path, Conf *conf);

#endif // _CONF_H_ 

