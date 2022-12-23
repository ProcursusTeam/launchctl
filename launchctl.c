/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Procursus Team <team@procurs.us>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <xpc/xpc.h>

#include "xpc_private.h"
#include "os_alloc_once.h"

#include "launchctl.h"

__SCCSID("@(#)PROGRAM:launchctl  PROJECT:ProcursusTeam/launchctl  VERSION:0.1beta2");

// TODO: Improve the help to support showing more information
// Ex. `launchctl help load`
static const struct {
	const char *name;
	const char *description;
	const char *usage;
	cmd_main *exec;
} cmds[] = {
	{ "bootstrap", "Bootstraps a domain or a service into a domain.", "<domain-target> [service-path, service-path2, ...]", bootstrap_cmd },
	{ "bootout", "Tears down a domain or removes a service from a domain.", "<domain-target> [service-path1, service-path2, ...] | <service-target>", bootout_cmd },
	{ "enable", "Enables an existing service.", "<service-target>", enable_cmd },
	{ "disable", "Disables an existing service.", "<service-target>", enable_cmd },
	{ "print", "Prints a description of a domain or service.", "<domain-target> | <service-target>", print_cmd },
	{ "reboot", "Initiates a system reboot of the specified type.", "[system|halt|obliterate|userspace] [-s]", reboot_cmd },
	{ "load", "Bootstraps a service or directory of services.", "<service-path, service-path2, ...>", load_cmd },
	{ "unload", "Unloads a service or directory of services.", "<service-path, service-path2, ...>", load_cmd },
	{ "list", "Lists information about services.", "[service-name]", list_cmd },
	{ "start", "Starts the specified service.", "<service-name>", start_cmd },
	{ "stop", "Stops the specified service if it is running.", "<service-name>", stop_cmd },
	{ "setenv", "Sets the specified environment variables for all services within the domain.", "<<key> <value>, ...>", setenv_cmd },
	{ "unsetenv", "Unsets the specified environment variables for all services within the domain.", "<key, ...>", setenv_cmd },
	{ "getenv", "Gets the value of an environment variable from within launchd.", "<key>", getenv_cmd },
	{ "error", "Prints a description of an error.", "[posix|mach|bootstrap] <code>", error_cmd },
	{ "variant", "Prints the launchd variant.", NULL, version_cmd },
	{ "version", "Prints the launchd version.", NULL, version_cmd },
	{ "help", "Prints the usage for a given subcommand.", "<subcommand>", help_cmd }
};

int
main(int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t msg;
	const char *name = NULL;
	if (argc <= 1) {
		help_cmd(NULL, argc - 1, argv + 1, envp, apple);
		return 0;
	}

	int ret = 0;
	int n = sizeof(cmds)/sizeof(cmds[0]);
	for (int i = 0; i < n; i++) {
		if (strcmp(argv[1], cmds[i].name) == 0) {
			ret = (cmds[i].exec)(&msg, argc - 1, argv + 1, envp, apple);
			goto finish;
		}
	}
	fprintf(stderr, "Unrecognized subcommand: %s\n", argv[1]);
	help_cmd(NULL, 0, NULL, NULL, NULL);
	return 1;

finish:
	switch (ret) {
		case ENODOMAIN:
			fprintf(stderr, "Could not find domain for ");
			launchctl_print_domain_str(stderr, msg);
			fprintf(stderr, "\n");
			break;
		case ENOSERVICE:
			name = xpc_dictionary_get_string(msg, "name");
			if (name == NULL)
				fprintf(stderr, "Could not find service.\n");
			else {
				fprintf(stderr, "Could not find service \"%s\" in domain for ", name);
				launchctl_print_domain_str(stderr, msg);
				fprintf(stderr, "\n");
			}
			break;
		case E2BIMPL:
			fprintf(stderr, "Command is not yet implemented.\n");
			break;
		case EBADNAME:
			fprintf(stderr, "Unrecognized target specifier. <service-target> takes a form of <domain-target>/<service-id>.\n");
			fprintf(stderr, "Please refer to `man launchctl` for explanation of the <domain-target> specifiers.\n");
		case EUSAGE:
			help_cmd(NULL, argc, argv, NULL, NULL);
			return 64;
	}
	return ret;
}

int
list_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t reply;
	char *label = NULL;
	if (argc >= 2)
		label = argv[1];

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	launchctl_setup_xpc_dict(dict);
	if (label != NULL)
		xpc_dictionary_set_string(dict, "name", label);

	int ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_LIST, dict, &reply);
	if (ret != 0)
		return ret;

	if (label == NULL) {
		xpc_object_t services = xpc_dictionary_get_value(reply, "services");
		if (services == NULL)
			return EBADRESP;
		printf("PID\tStatus\tLabel\n");
		(void)xpc_dictionary_apply(services, ^ bool (const char *key, xpc_object_t value) {
				int64_t pid = xpc_dictionary_get_int64(value, "pid");
				if (pid == 0)
					printf("-\t");
				else
					printf("%"PRId64"\t", pid);
				int64_t status = xpc_dictionary_get_int64(value, "status");
				if (WIFSTOPPED(status))
					printf("???\t%s\n", key);
				else if (WIFEXITED(status))
					printf("%d\t%s\n", WEXITSTATUS(status), key);
				else if (WIFSIGNALED(status))
					printf("-%d\t%s\n", WTERMSIG(status), key);
				return true;
		});
	} else {
		xpc_object_t service = xpc_dictionary_get_dictionary(reply, "service");
		if (service == NULL)
			return EBADRESP;

		launchctl_xpc_object_print(service, NULL, 0);
	}
	return 0;
}

int
version_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t reply;
	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	launchctl_setup_xpc_dict(dict);
	vm_address_t addr = 0;
	vm_size_t sz = 0x100000;

	if (__isPlatformVersionAtLeast(2, 15, 0, 0)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}

	if (strcmp(argv[0], "variant") == 0)
		xpc_dictionary_set_bool(dict, "variant", 1);
	else
		xpc_dictionary_set_bool(dict, "version", 1);

	int ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret == EINVAL) {
		fprintf(stderr, "Bad request.\n");
	} else if (ret != 0) {
		fprintf(stderr, "Could not print variant: %d: %s\n", ret, xpc_strerror(ret));
	}

	if (__isPlatformVersionAtLeast(2, 15, 0, 0))
		launchctl_print_shmem(reply, addr, sz, stdout);

	if (addr != 0)
		vm_deallocate(mach_task_self(), addr, sz);

	return ret;
}

int
help_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc > 1) {
		fprintf(stderr, "Usage: %s ", getprogname());
		int n = sizeof(cmds)/sizeof(cmds[0]);
		for (int i = 0; i < n; i++) {
			if (strcmp(cmds[i].name, argv[1]) == 0) {
				fprintf(stderr, "%s %s\n", cmds[i].name, cmds[i].usage == NULL ? "" : cmds[i].usage);
				return 0;
			}
		}
		fprintf(stderr, "help <subcommand>\n");
		return 64;
	}
	printf("Usage: %s <subcommand> ... | help [subcommand]\n"
		"Many subcommands take a target specifier that refers to a domain or service\n"
		"within that domain. The available specifier forms are:\n"
		"\n"
		"system/[service-name]\n"
		"Targets the system-wide domain or service within. Root privileges are required\n"
		"to make modifications.\n"
		"\n"
		"user/<uid>/[service-name]\n"
		"Targets the user domain or service within. A process running as the target user\n"
		"may make modifications. Root may modify any user's domain. User domains do not\n"
		"exist on iOS.\n"
		"\n"
		"gui/<uid>/[service-name]\n"
		"Targets the GUI domain or service within. Each GUI domain is associated with a\n"
		"user domain, and a process running as the owner of that user domain may make\n"
		"modifications. Root may modify any GUI domain. GUI domains do not exist on iOS.\n"
		"\n"
		"session/<asid>/[service-name]\n"
		"Targets a session domain or service within. A process running within the target\n"
		"security audit session may make modifications. Root may modify any session\n"
		"domain.\n"
		"\n"
		"pid/<pid>/[service-name]\n"
		"Targets a process domain or service within. Only the process which owns the\n"
		"domain may modify it. Even root may not do so.\n"
		"\n"
		"When using a legacy subcommand which manipulates a domain, the target domain is\n"
		"assumed to be the system domain. On iOS, there is no support for per-user\n"
		"domains, even though there is a mobile user.\n", getprogname());
	printf("\nSubcommands:\n");
	int n = sizeof(cmds)/sizeof(cmds[0]);
	for (int i = 0; i < n; i++) {
		printf("\t%-16s%s\n", cmds[i].name, cmds[i].description);
	}
	return 0;
}

void
launchctl_print_domain_str(FILE *s, xpc_object_t msg)
{
	uint64_t type, handle;

	type = xpc_dictionary_get_uint64(msg, "type");
	handle = xpc_dictionary_get_uint64(msg, "handle");

	switch (type) {
		case 1:
			fprintf(s, "system");
			break;
		case 2:
			fprintf(s, "uid: %"PRIu64, handle);
			break;
		case 3:
			fprintf(s, "login: %"PRIu64, handle);
			break;
		case 4:
			fprintf(s, "asid: %"PRIu64, handle);
			break;
		case 5:
			fprintf(s, "pid: %"PRIu64, handle);
			break;
		case 7:
			fprintf(s, "ports");
			break;
		case 8:
			fprintf(s, "user gui: %"PRIu64, handle);
			break;
	}
}
