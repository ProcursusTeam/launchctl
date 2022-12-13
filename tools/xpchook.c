#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>
typedef xpc_object_t xpc_pipe_t;

#include <Availability.h>

#define DYLD_INTERPOSE(_replacement,_replacee) \
   __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
            __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };

kern_return_t
xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t *reply);

kern_return_t
hook_xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t *reply)
{
	kern_return_t ret = xpc_pipe_routine(pipe, request, reply);
	char *requeststr = xpc_copy_description(request);
	fprintf(stderr, "\033[32mREQUEST: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_routine, xpc_pipe_routine);

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 130000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500
kern_return_t
xpc_pipe_routine_with_flags(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t *reply, uint32_t flags);

kern_return_t
hook_xpc_pipe_routine_with_flags(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t *reply, uint32_t flags)
{
	kern_return_t ret = xpc_pipe_routine_with_flags(pipe, request, reply, flags);
	char *requeststr = xpc_copy_description(request);
	fprintf(stderr, "\033[32mREQUEST: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	fprintf(stderr, "\033[32mFLAGS: %i\033[m\n", flags);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_routine_with_flags, xpc_pipe_routine_with_flags);
#endif

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 160000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 130000
int _xpc_pipe_interface_routine(xpc_pipe_t pipe, uint64_t request, xpc_object_t msg, xpc_object_t *reply, uint64_t unknown);

int
hook_xpc_pipe_interface_routine(xpc_pipe_t pipe, uint64_t request, xpc_object_t msg, xpc_object_t *reply, uint64_t flags)
{
	int ret = _xpc_pipe_interface_routine(pipe, request, msg, reply, flags);
	fprintf(stderr, "\033[32mREQUEST: %llu\033[m\n", request);
	char *requeststr= xpc_copy_description(msg);
	fprintf(stderr, "\033[32mMSG: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	fprintf(stderr, "\033[32mFLAGS: %llu\033[m\n", flags);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_interface_routine, _xpc_pipe_interface_routine);
#endif
