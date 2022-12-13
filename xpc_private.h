#include <errno.h>
#include <xpc/xpc.h>

#ifndef _XPC_PRIVATE_H_
#define _XPC_PRIVATE_H_

enum {
	XPC_ROUTINE_PRINT_SERVICE = 708,
	XPC_ROUTINE_LOAD = 800,
	XPC_ROUTINE_UNLOAD,
	XPC_ROUTINE_ENABLE = 808,
	XPC_ROUTINE_DISABLE,
	XPC_ROUTINE_SERVICE_KILL = 812,
	XPC_ROUTINE_SERVICE_START,
	XPC_ROUTINE_SERVICE_STOP,
	XPC_ROUTINE_LIST,
	XPC_ROUTINE_SETENV = 819,
	XPC_ROUTINE_GETENV,
	XPC_ROUTINE_PRINT = 828,
};

typedef xpc_object_t xpc_pipe_t;

int xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t message, xpc_object_t XPC_GIVES_REFERENCE *reply);

const char *xpc_strerror(int);

#define XPC_TYPE_MACH_SEND (&_xpc_type_mach_send)
XPC_EXPORT
XPC_TYPE(_xpc_type_mach_send);

typedef void (*xpc_dictionary_applier_f)(const char *key, xpc_object_t val, void *ctx);
void xpc_dictionary_apply_f(xpc_object_t xdict, void *ctx, xpc_dictionary_applier_f applier);

enum {
	ENODOMAIN = 112,
	ENOSERVICE,
	E2BIMPL = 116,
	EUSAGE,
	EBADRESP,
	EMANY = 133,
	EBADNAME = 140,
};

#endif
