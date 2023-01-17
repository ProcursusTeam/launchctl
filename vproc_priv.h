#include <sys/types.h>

#ifndef _LAUNCHCTL_VPROC_PRIV_H_
#define _LAUNCHCTL_VPROC_PRIV_H_

typedef enum {
	VPROC_GSK_MGR_UID = 3,
	VPROC_GSK_MGR_PID = 4,
	VPROC_GSK_MGR_NAME = 6,
} vproc_gsk_keys;

void *vproc_swap_string(void *, vproc_gsk_keys, const char *, char **);
void *vproc_swap_integer(void *, vproc_gsk_keys, int64_t, int64_t *);

#endif
