#ifndef _CMD_H_
#define _CMD_H_

#include "argiter.h"

typedef bool(*CmdFunc)(ArgIter *args, void *cmd_data);

typedef struct CmdFlagInfo {
	union {
		struct {
			const char *short_name;
			const char *long_name;
		};
		const char *names[2];
	};
	CmdFunc cmd;
} CmdFlagInfo;

// Commands
bool cmd_new(ArgIter *args, const char *bx_path);
bool cmd_build(ArgIter *args);
bool cmd_run(ArgIter *args);

#endif

