#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sysdir.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
load_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	bool load = strcmp(argv[0], "load") == 0;
	int ret;
	unsigned int domain = 0;
	bool wflag, force;
	wflag = force = false;

	int ch;
	while ((ch = getopt(argc, argv, "wFS:D:")) != -1) {
		switch (ch) {
			case 'w':
				wflag = true;
				break;
			case 'F':
				force = true;
				break;
			case 'D':
				if (strcasecmp(optarg, "all") == 0) {
					domain |= SYSDIR_DOMAIN_MASK_ALL;
				} else if (strcasecmp(optarg, "user") == 0) {
					domain |= SYSDIR_DOMAIN_MASK_USER;
				} else if (strcasecmp(optarg, "local") == 0) {
					domain |= SYSDIR_DOMAIN_MASK_LOCAL;
				} else if (strcasecmp(optarg, "network") == 0) {
					fprintf(stderr, "Ignoring network domain.\n");
				} else if (strcasecmp(optarg, "system") == 0) {
					domain |= SYSDIR_DOMAIN_MASK_SYSTEM;
				} else {
					return EUSAGE;
				}
				break;
			case 'S':
				fprintf(stderr, "Session types are not supported on iOS. Ignoring session specifier: %s\n", optarg);
				break;
			default:
				return EUSAGE;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	launchctl_setup_xpc_dict(dict);
	xpc_object_t array = launchctl_parse_load_unload(domain, argc, argv);
	xpc_dictionary_set_value(dict, "paths", array);
	if (load)
		xpc_dictionary_set_bool(dict, "enable", wflag);
	else
		xpc_dictionary_set_bool(dict, "disable", wflag);
	xpc_dictionary_set_bool(dict, "legacy-load", true);
	if (force)
		xpc_dictionary_set_bool(dict, "force", true);
	ret = launchctl_send_xpc_to_launchd(load ? XPC_ROUTINE_LOAD : XPC_ROUTINE_UNLOAD, dict, &reply);
	if (ret == 0) {
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
			
		}
	}

	return ret;
}
