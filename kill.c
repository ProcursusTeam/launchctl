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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <xpc/xpc.h>
#include "xpc_private.h"

#include "launchctl.h"

int
kill_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 3)
		return EUSAGE;

	xpc_object_t dict, reply;
	const char *name;
	int err, sig;

	char *sign = argv[1];
	if (*sign == '-')
		sign++;

	if (strlen(sign) > 3 && strncasecmp(sign, "sig", 3) == 0)
		sign += 3;

	sig = strtol(sign, NULL, 10);
	if (sig > NSIG) {
		fprintf(stderr, "Invalid signal.\n");
		return EUSAGE;
	}

	if (sig < 2) {
		sig = 1;
		while (sig != NSIG) {
			if (strcasecmp(sys_signame[sig], sign) == 0)
				goto cont;
			sig++;
		}
		fprintf(stderr, "Invalid signal.\n");
		return EUSAGE;
	}

cont:
	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	if ((err = launchctl_setup_xpc_dict_for_service_name(argv[2], dict, &name)) != 0)
		return err;

	if (name == NULL)
		return EBADNAME;

	xpc_dictionary_set_int64(dict, "signal", sig);
	xpc_dictionary_set_string(dict, "name", name);
	err = launchctl_send_xpc_to_launchd(XPC_ROUTINE_SERVICE_KILL, dict, &reply);

	if (err == EPERM)
		fprintf(stderr, "Not privileged to signal service.\n");
	else if (err == ESRCH)
		fprintf(stderr, "No process to signal.\n");

	return err;
}
