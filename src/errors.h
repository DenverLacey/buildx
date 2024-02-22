#ifndef _ERRORS_H_
#define _ERRORS_H_

typedef enum ResultCode {
	OK = 0,
	INTERNAL_ERROR,
	NO_ARG_VALUE,
	BAD_ARG_VALUE,
	FAIL_CREATE_DIRECTORY,
	FAIL_OPEN_FILE,
	FAIL_SET_PERMS
} ResultCode;

#endif

