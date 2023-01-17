/*-
 * SPDX-License-Identifier: BSD 2-Clause License
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
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>

#include <xpc/xpc.h>
#include "xpc_private.h"

#include "launchctl.h"

int
examine_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc == 2)
		return EUSAGE;
	int ret;
	xpc_object_t dict, reply;
	dict = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_uint64(dict, "type", 1);
	xpc_dictionary_set_uint64(dict, "handle", 0);
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_EXAMINE, dict, &reply);
	if (ret == ENOTDEVELOPMENT) {
		fprintf(stderr, "Examination is only available on the DEVELOPMENT variant.\n");
		return ret;
	} else if (ret == ENOTSUP) {
		fprintf(stderr, "Examination is not available on this platform.\n");
		return ret;
	} else if (ret != 0) {
		return ret;
	}
	int64_t candidate_pid = xpc_dictionary_get_int64(reply, "pid");
	char candidate_pid_str[0x18];
	snprintf(candidate_pid_str, 0x18, "%" PRId64, candidate_pid);
	if (candidate_pid < 2) {
		return EBADRESP;
	}
	if (argc == 1) {
		printf("%" PRId64 "\n", candidate_pid);
		return 0;
	}
	for (uint64_t i = 2; argv[i] != NULL; i++) {
		if (strcmp(argv[i], "@PID") != 0) continue;
		argv[i] = candidate_pid_str;
	}
	pid_t examiner_pid = 0;
	int examiner_status = 0;
	int pspawn_ret = posix_spawnp(&examiner_pid, argv[1], NULL, NULL, &argv[1], envp);
	if (pspawn_ret != 0) {
		fprintf(stderr, "posix_spawnp(): %d: %s\n", pspawn_ret, strerror(pspawn_ret));
		return ret;
	}
	pid_t waitpid_ret = waitpid(examiner_pid, &examiner_status, 0);
	if (waitpid_ret != examiner_pid) {
		fprintf(stderr, "waitpid(): %d: %s\n", errno, strerror(errno));
		return ret;
	}
	int kill_ret = kill(candidate_pid, SIGKILL);
	if (kill_ret != 0) {
		fprintf(stderr, "kill(): %d: %s\n", errno, strerror(errno));
		return ret;
	}
	return 0;
}
