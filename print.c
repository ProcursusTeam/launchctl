#include <errno.h>
#include <stdio.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
print_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;
	int ret = EUSAGE;
	xpc_object_t reply;
	const char *name = NULL;

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	if (launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name) != 0)
		return EUSAGE;

	xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	if (name != NULL)
		ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT_SERVICE, dict, &reply);
	else
		ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret > ENODOMAIN) {
		if (ret == EINVAL)
			fprintf(stderr, "Bad request.\n");
		else
			fprintf(stderr, "Could not print domain: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}
