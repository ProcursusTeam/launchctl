/*-
 * SPDX-License-Identifier: BSD 2-Clause License
 *
 * Copyright (c) 2024 Procursus Team <team@procurs.us>
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
#include <mach/mach.h>
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
resolveport_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 3)
		return EUSAGE;

	pid_t pid = strtol(argv[1], NULL, 0);
	mach_port_t port = strtol(argv[2], NULL, 0);

	mach_port_t task = 0;
	kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

	if (kr) {
		fprintf(stderr, "task_for_pid(): 0x%x\n", kr);
		return kr;
	}

	mach_port_type_t type;

	kr = mach_port_type(task, port, &type);

	if (kr) {
		if (kr == KERN_INVALID_NAME)
			fprintf(stderr, "Right name does not exist in task.\n");
		else
			fprintf(stderr, "task_for_pid(): 0x%x\n", kr);

		return kr;
	}

	char rights[256] = { 0 };

#define RIGHT_DESCRIPTION(desc, bit)               \
	if (type & bit) {                          \
		if (rights[0])                     \
			strlcat(rights, "/", 256); \
		strlcat(rights, desc, 256);        \
	}

	RIGHT_DESCRIPTION("receive", MACH_PORT_TYPE_RECEIVE);
	RIGHT_DESCRIPTION("send", MACH_PORT_TYPE_SEND);
	RIGHT_DESCRIPTION("send-once", MACH_PORT_TYPE_SEND_ONCE);
	RIGHT_DESCRIPTION("dead name", MACH_PORT_TYPE_DEAD_NAME);
	RIGHT_DESCRIPTION("port set", MACH_PORT_RIGHT_PORT_SET);
	RIGHT_DESCRIPTION("mac label", MACH_PORT_RIGHT_LABELH);
	RIGHT_DESCRIPTION("dead-name request", MACH_PORT_TYPE_DNREQUEST);
	RIGHT_DESCRIPTION("send-possible request", MACH_PORT_TYPE_SPREQUEST);
	RIGHT_DESCRIPTION("delayed send-possible request", MACH_PORT_TYPE_SPREQUEST_DELAYED);

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0), reply;
	*msg = dict;

	launchctl_setup_xpc_dict_for_service_name("system", dict, NULL);
	xpc_dictionary_set_int64(dict, "process", pid);
	xpc_dictionary_set_uint64(dict, "name", port);

	int retval = launchctl_send_xpc_to_launchd(XPC_ROUTINE_RESOLVE_PORT, dict, &reply);
	if (retval == ESRCH) {
		fprintf(stderr, "Could not find name 0x%x in %d\n", port, pid);
		return ESRCH;
	} else if (retval == ENOSERVICE) {
		fprintf(stderr, "Could not obtain task port for PID: %d\n", pid);
		return ESRCH;
	} else if (retval) {
		return retval;
	}

	if (xpc_dictionary_get_uint64(reply, "kern-error")) {
		fprintf(stderr, "Could not obtain right for port: 0x%x\n", port);
		return 0;
	}

	const char *domain = xpc_dictionary_get_string(reply, "domain");
	if (!domain)
		return EBADRESP;

	printf("domain = %s\n", domain);

	const char *service = xpc_dictionary_get_string(reply, "service");
	const char *endpoint = xpc_dictionary_get_string(reply, "endpoint");

	if (service)
		printf("service = %s\n", service);

	if (endpoint)
		printf("endpoint = %s\n", endpoint);

	printf("task rights = %s\n", rights);

	return 0;
}
