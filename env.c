#include <errno.h>
#include <stdio.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
setenv_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t dict, env, reply;
	int ret;
	bool setenv = strcmp(argv[0], "setenv") == 0 ? true : false;

	argc--;
	argv++;

	if (setenv && argc % 2 != 0)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	*msg = dict;
	env = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_value(dict, "envvars", env);

	for (int i = 0; i < argc; i++) {
		if (setenv) {
			xpc_dictionary_set_string(env, argv[i], argv[i + 1]);
			i++;
		} else {
			xpc_object_t null = xpc_null_create();
			xpc_dictionary_set_value(env, argv[i], null); 
			xpc_release(null);
		}
	}

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SETENV, dict, &reply);
	if (ret != 0) {
		if (ret == EPERM)
			fprintf(stderr, "Not privileged to set domain environment.\n");
		else {
			fprintf(stderr, "Could not set environment: %d: %s\n", ret, xpc_strerror(ret));
		}
	}

	return ret;
}

int
getenv_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	const char *val = NULL;
	xpc_object_t dict, reply;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	*msg = dict;
	xpc_dictionary_set_string(dict, "envvar", argv[1]);

	if (launchctl_send_xpc_to_launchd(XPC_ROUTINE_GETENV, dict, &reply) == 0) {
		val = xpc_dictionary_get_string(reply, "value");
		if (val == NULL)
			return EBADRESP;
		printf("%s\n", val);
	}

	return 0;
}
