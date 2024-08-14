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
#include <inttypes.h>
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

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
		(void)xpc_dictionary_apply(services, ^bool(const char *key, xpc_object_t value) {
		    int64_t pid = xpc_dictionary_get_int64(value, "pid");
		    if (pid == 0)
			    printf("-\t");
		    else
			    printf("%" PRId64 "\t", pid);
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
