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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
setenv_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t dict, env, reply;
	int ret;
	bool setenv = strcmp(argv[0], "setenv") == 0 ? true : false;

	argc--;
	argv++;

	if (setenv && argc % 2 != 0)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	*msg = dict;
	env = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_value(dict, "envvars", env);

	for (int i = 0; i < argc; i++) {
		if (setenv) {
			xpc_dictionary_set_string(env, argv[i], argv[i + 1]);
			i++;
		} else {
			xpc_object_t null = xpc_null_create();
			xpc_dictionary_set_value(env, argv[i], null);
			xpc_release(null);
		}
	}

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SETENV, dict, &reply);
	if (ret != 0) {
		if (ret == EPERM)
			fprintf(stderr, "Not privileged to set domain environment.\n");
		else {
			fprintf(stderr, "Could not set environment: %d: %s\n", ret, xpc_strerror(ret));
		}
	}

	return ret;
}

int
getenv_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	const char *val = NULL;
	xpc_object_t dict, reply;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict(dict);
	*msg = dict;
	xpc_dictionary_set_string(dict, "envvar", argv[1]);

	if (launchctl_send_xpc_to_launchd(XPC_ROUTINE_GETENV, dict, &reply) == 0) {
		val = xpc_dictionary_get_string(reply, "value");
		if (val == NULL)
			return EBADRESP;
		printf("%s\n", val);
	}

	return 0;
}
