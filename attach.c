/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Procursus Team <team@procurs.us>
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
#include <sys/wait.h>

#include <crt_externs.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
attach_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	const char *name;
	int err;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	int ch;
	while ((ch = getopt(argc, argv, "ksx")) != -1) {
		switch (ch) {
			case 'k':
				xpc_dictionary_set_bool(dict, "kill", true);
				break;
			case 's':
				xpc_dictionary_set_bool(dict, "run", true);
				break;
			case 'x':
				xpc_dictionary_set_bool(dict, "proxy", true);
				break;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		return EUSAGE;

	if ((err = launchctl_setup_xpc_dict_for_service_name(argv[0], dict, &name)) != 0)
		return err;
	if (name == NULL)
		return EBADNAME;

	err = launchctl_send_xpc_to_launchd(XPC_ROUTINE_ATTACH_SERVICE, dict, &reply);

	if (err == EINVAL) {
		fprintf(stderr, "Bad request.\n");
		return err;
	} else if (err != 0) {
		fprintf(stderr, "Could not attach to service \"%s\": %d: %s\n", name, err, xpc_strerror(err));
		return err;
	}

	int64_t pid = xpc_dictionary_get_int64(reply, "pid");
	if (pid == 0)
		return EBADRESP;

	printf("Attaching to pid: %" PRId64 "\n", pid);

	char pidstr[24];
	snprintf(pidstr, sizeof(pidstr), "%" PRId64, pid);

	char *lldbargv[7] = { "xcrun", "lldb", "process", "attach", "-p", pidstr, NULL };

	unsetenv("DYLD_ROOT_PATH");
	unsetenv("DYLD_FALLBACK_FRAMEWORK_PATH");
	unsetenv("DYLD_FALLBACK_LIBRARY_PATH");
	// setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1); Leave this disabled due to rootless PATHs

	pid_t lldbpid;
	err = posix_spawnp(&lldbpid, "lldb", NULL, NULL, lldbargv, *_NSGetEnviron());

	if (err != 0) {
		fprintf(stderr, "Could not attach to service: %d: %s\n", err, xpc_strerror(err));
		return err;
	}

	int status;

	if (waitpid(lldbpid, &status, 0) == -1) {
		fprintf(stderr, "waitpid(2): %d: %s\n", errno, strerror(errno));
	} else {
		if (WIFEXITED(status)) {
			if (WEXITSTATUS(status) == 0)
				return 0;
			fprintf(stderr, "lldb exited with code: %d\n", WEXITSTATUS(status));
		} else if (WIFSIGNALED(status)) {
			fprintf(stderr, "lldb terminated by signal: %s\n", strsignal(WTERMSIG(status)));
		}
	}

	return 0;
}
