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
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
stop_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	int ret;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	xpc_dictionary_set_string(dict, "name", argv[1]);
	*msg = dict;
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SERVICE_STOP, dict, &reply);
	if (ret == EPERM) {
		fprintf(stderr, "Not privileged to stop service.\n");
	}
	return ret == EALREADY ? 0 : ret;
}

int
start_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	int ret;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	xpc_dictionary_set_string(dict, "name", argv[1]);
	*msg = dict;
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SERVICE_START, dict, &reply);
	if (ret == EPERM) {
		fprintf(stderr, "Not privileged to start service.\n");
	}
	return ret == EALREADY ? 0 : ret;
}
