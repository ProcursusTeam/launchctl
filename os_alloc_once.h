#include <sys/types.h>
#include <xpc/xpc.h>

#define OS_ALLOC_ONCE_KEY_LIBXPC 1

struct xpc_global_data {
	uint64_t a;
	uint64_t xpc_flags;
	mach_port_t task_bootstrap_port; /* 0x10 */
#ifndef _64
	uint32_t padding;
#endif
	xpc_object_t xpc_bootstrap_pipe; /* 0x18 */
};

#define OS_ALLOC_ONCE_KEY_MAX 100

struct _os_alloc_once_s {
	long once;
	void *ptr;
};

extern struct _os_alloc_once_s _os_alloc_once_table[];
