#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sysdir.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
enable_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply, names;
	bool enable = strcmp(argv[0], "enable") == 0;
	int ret;
	const char *name;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	launchctl_setup_xpc_dict(dict);
	ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name);
	if (ret != 0)
		return ret;
	if (name == NULL)
		return EBADNAME;

	names = xpc_array_create(NULL, 0);
	xpc_dictionary_set_value(dict, "names", names);
	xpc_array_set_string(names, XPC_ARRAY_APPEND, name);

	ret = launchctl_send_xpc_to_launchd(enable ? XPC_ROUTINE_ENABLE : XPC_ROUTINE_DISABLE, dict, &reply);
	if (ret == ENODOMAIN)
		return ret;
	if (ret != 0) {
		fprintf(stderr, "Could not %s service: %d: %s\n", enable ? "enable" : "disable", ret, xpc_strerror(ret));
	} else {
		xpc_object_t errors = xpc_dictionary_get_value(reply, "errors");
		if (errors != NULL && xpc_get_type(errors) == XPC_TYPE_DICTIONARY) {
			(void)xpc_dictionary_apply(errors, ^bool(const char *key, xpc_object_t value) {
					if (xpc_get_type(value) == XPC_TYPE_INT64) {
						int64_t err = xpc_int64_get_value(value);
						if (err == EEXIST || err == EALREADY)
							fprintf(stderr, "%s: service already loaded\n", key);
						else
							fprintf(stderr, "%s: %s\n", key, xpc_strerror(err));
					}
					return true;
			});
			if (xpc_dictionary_get_count(errors) != 0)
				ret = EMANY;
		}
	}

	return ret;
}
