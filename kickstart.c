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
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
kickstart_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	bool printpid = false;
	const char *name = NULL;
	int err;
	int64_t pid;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	int ch;
	while ((ch = getopt(argc, argv, "pks")) != -1) {
		switch (ch) {
			case 'k':
				xpc_dictionary_set_bool(dict, "kill", true);
				break;
			case 's':
				xpc_dictionary_set_bool(dict, "suspended", true);
				break;
			case 'u':
				xpc_dictionary_set_bool(dict, "unthrottle", true);
				break;
			case 'p':
				printpid = true;
				break;
		}
	}

	if (argc <= optind)
		return EUSAGE;

	if ((err = launchctl_setup_xpc_dict_for_service_name(argv[optind], dict, &name)) != 0)
		return err;
	if (name == NULL)
		return EBADNAME;

	err = launchctl_send_xpc_to_launchd(XPC_ROUTINE_KICKSTART_SERVICE, dict, &reply);

	if (err == 0) {
		if (printpid) {
			pid = xpc_dictionary_get_int64(reply, "pid");
			if (isatty(STDOUT_FILENO)) {
				printf("service spawned with pid: %" PRId64 "\n", pid);
			} else {
				printf("%" PRId64 "\n", pid);
			}
		}
	} else if (err == EINVAL) {
		fprintf(stderr, "Bad request.\n");
	} else if (err != EALREADY && err < ENODOMAIN) {
		fprintf(stderr, "Could not kickstart service \"%s\": %d: %s\n", name, err, xpc_strerror(err));
	}

	return err;
}
