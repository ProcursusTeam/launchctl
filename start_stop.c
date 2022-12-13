#include <errno.h>
#include <stdio.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
stop_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	int ret;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	xpc_dictionary_set_string(dict, "name", argv[1]);
	*msg = dict;
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SERVICE_STOP, dict, &reply);
	if (ret == EPERM) {
		fprintf(stderr, "Not privileged to stop service.\n");
	}
	return ret == EALREADY ? 0 : ret;
}

int
start_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	int ret;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	xpc_dictionary_set_string(dict, "name", argv[1]);
	*msg = dict;
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SERVICE_START, dict, &reply);
	if (ret == EPERM) {
		fprintf(stderr, "Not privileged to start service.\n");
	}
	return ret == EALREADY ? 0 : ret;
}
