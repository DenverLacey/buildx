#ifndef _CMD_H_
#define _CMD_H_

#include "argiter.h"
#include "errors.h"

typedef ResultCode(*CmdFunc)(ArgIter *args);

typedef struct CmdFlagInfo {
	union {
		struct {
			const char *short_name;
			const char *long_name;
		};
		const char *const names[2];
	};
	CmdFunc cmd;
} CmdFlagInfo;

#endif

