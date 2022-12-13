#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sysdir.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
bootstrap_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	const char *name = NULL;
	xpc_object_t dict, reply, paths;

	if (argc < 2)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name);
	if (ret != 0)
		return ret;
	if (name != NULL)
		return EBADNAME;

	if (argc > 2) {
		paths = launchctl_parse_load_unload(0, argc - 2, argv + 2);
		xpc_dictionary_set_value(dict, "paths", paths);
	}
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_LOAD, dict, &reply);
	if (ret != ENODOMAIN) {
		if (ret == 0) {
			xpc_object_t errors;
			errors = xpc_dictionary_get_value(reply, "errors");
			if (errors == NULL || xpc_get_type(errors) != XPC_TYPE_DICTIONARY)
				return 0;
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
				return EMANY;
			else
				return 0;
		}
		fprintf(stderr, "Bootstrap failed: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}

int
bootout_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	const char *name = NULL;
	xpc_object_t dict, reply, paths;

	if (argc < 2)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name);
	if (ret != 0)
		return ret;

	if (argc > 2 && name == NULL) {
		paths = launchctl_parse_load_unload(0, argc - 2, argv + 2);
		xpc_dictionary_set_value(dict, "paths", paths);
	}
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_UNLOAD, dict, &reply);
	if (ret != ENODOMAIN) {
		if (ret == 0) {
			xpc_object_t errors;
			errors = xpc_dictionary_get_value(reply, "errors");
			if (errors == NULL || xpc_get_type(errors) != XPC_TYPE_DICTIONARY)
				return 0;
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
				return EMANY;
			else
				return 0;
		}
		fprintf(stderr, "Boot-out failed: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}
