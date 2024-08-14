/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022-2023 Procursus Team <team@procurs.us>
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
#include <sys/cdefs.h>
#include <sys/wait.h>

#include <errno.h>
#include <inttypes.h>
#include <mach/mach.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

__SCCSID("@(#)PROGRAM:launchctl  PROJECT:ProcursusTeam/launchctl  VERSION:1.1.1");

char launchctl __attribute__((section("__TEXT,__launchctl"))) = 1;

// clang-format off
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
	{ "uncache", "Removes the specified service name from the service cache.", "<service-name>", uncache_cmd },
	{ "kickstart", "Forces an existing service to start.", "[-k] [-p] <service-target>", kickstart_cmd },
	{ "attach", "Attach the system's debugger to a service.", "[-k] [-s] [-x] <service-target>", attach_cmd },
	{ "debug", "Configures the next invocation of a service for debugging.", "<service-target> [--program <program-path>] [--start-suspended] [oc-stack-logging] [--malloc-nano-allocator] [--debug-libraries] [--NSZombie] [--32] [--stdin [path]] [--stdout [path]] [--stderr [path]] [--environment VARIABLE0=value0 VARIABLE1=value1 ...] -- [argv0 argv1 ...]", todo_cmd },
	{ "kill", "Sends a signal to the service instance.", "<signal-number|signal-name> <service-target>", kill_cmd },
	{ "blame", "Prints the reason a service is running.", "<service-target>", blame_cmd },
	{ "print", "Prints a description of a domain or service.", "<domain-target> | <service-target>", print_cmd },
	{ "print-cache", "Prints information about the service cache.", NULL, print_cache_cmd },
	{ "print-disabled", "Prints which services are disabled.", NULL, print_disabled_cmd },
	{ "plist", "Prints a property list embedded in a binary (targets the Info.plist by default).", "[segment,section] <path>", plist_cmd },
	{ "procinfo", "Prints port information about a process.", "<pid>", procinfo_cmd },
	{ "hostinfo", "Prints port information about the host.", NULL, hostinfo_cmd },
	{ "resolveport", "Resolves a port name from a process to an endpoint in launchd.", "<owner-pid> <port-name>", todo_cmd },
	{ "limit", "Reads or modifies launchd's resource limits.", "[<limit-name> [<both-limits> | <soft-limit> <hard-limit>]", limit_cmd },
	{ "runstats", "Prints performance statistics for a service.", "<service-target>", runstats_cmd },
	{ "examine", "Runs the specified analysis tool against launchd in a non-reentrant manner.", "[<tool> [arg0, arg1, ... , @PID, ...]]", examine_cmd },
	{ "config", "Modifies persistent configuration parameters for launchd domains.", NULL, config_cmd },
	{ "dumpstate", "Dumps launchd state to stdout.", NULL, dumpstate_cmd },
	{ "dumpjpcategory", "Dumps the jetsam properties category for all services.", NULL, dumpjpcategory_cmd },
	{ "reboot", "Initiates a system reboot of the specified type.", "[system|halt|obliterate|userspace] [-s]", reboot_cmd },
	{ "userswitch", "Initiates a user switch.", "<old-uid> <new-uid>", userswitch_cmd },
	{ "load", "Bootstraps a service or directory of services.", "<service-path, service-path2, ...>", load_cmd },
	{ "unload", "Unloads a service or directory of services.", "<service-path, service-path2, ...>", load_cmd },
	{ "remove", "Unloads the specified service name.", "<service-name>", remove_cmd },
	{ "list", "Lists information about services.", "[service-name]", list_cmd },
	{ "start", "Starts the specified service.", "<service-name>", start_cmd },
	{ "stop", "Stops the specified service if it is running.", "<service-name>", stop_cmd },
	{ "setenv", "Sets the specified environment variables for all services within the domain.", "<<key> <value>, ...>", setenv_cmd },
	{ "unsetenv", "Unsets the specified environment variables for all services within the domain.", "<key, ...>", setenv_cmd },
	{ "getenv", "Gets the value of an environment variable from within launchd.", "<key>", getenv_cmd },
	{ "bsexec", "Execute a program in another process' bootstrap context.", "<pid> <program> [...]", todo_cmd },
	{ "asuser", "Execute a program in the bootstrap context of a given user.", "<uid> <program> [...]", todo_cmd },
	{ "submit", "Submit a basic job from the command line.", "-l <label> [-p <program>] [-o <stdout-path>] [-e <stderr-path] -- <command> [arg0, arg1, ...]", submit_cmd },
	{ "managerpid", "Prints the PID of the launchd controlling the session.", NULL, managerpid_cmd },
	{ "manageruid", "Prints the UID of the current launchd session.", NULL, manageruid_cmd },
	{ "managername", "Prints the name of the current launchd session.", NULL, managername_cmd },
	{ "error", "Prints a description of an error.", "[posix|mach|bootstrap] <code>", error_cmd },
	{ "variant", "Prints the launchd variant.", NULL, version_cmd },
	{ "version", "Prints the launchd version.", NULL, version_cmd },
	{ "help", "Prints the usage for a given subcommand.", "<subcommand>", help_cmd }
};
// clang-format on

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
	int n = sizeof(cmds) / sizeof(cmds[0]);
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
			fprintf(stderr,
			    "Unrecognized target specifier. <service-target> takes a form of <domain-target>/<service-id>.\n");
			fprintf(stderr,
			    "Please refer to `man launchctl` for explanation of the <domain-target> specifiers.\n");
		case EUSAGE:
			help_cmd(NULL, argc, argv, NULL, NULL);
			return 64;
	}
	return ret;
}

int
help_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc > 1) {
		fprintf(stderr, "Usage: %s ", getprogname());
		int n = sizeof(cmds) / sizeof(cmds[0]);
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
	       "domains, even though there is a mobile user.\n",
	    getprogname());
	printf("\nSubcommands:\n");
	int n = sizeof(cmds) / sizeof(cmds[0]);
	for (int i = 0; i < n; i++) {
		printf("\t%-16s%s\n", cmds[i].name, cmds[i].description);
	}
	return 0;
}

int
config_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	return ENOTSUP;
}

int
submit_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	return EDEPRECATED;
}

int
uncache_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	return E2BIMPL;
}

int
todo_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	return ENOTSUP;
}
