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
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

extern const char *bootstrap_strerror(kern_return_t);

int
error_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int retval = EUSAGE;

	const char *extraMessagesTable[] = {
		"kernel out-of-line resource shortage",
		"kernel ipc resource shortage",
		"unknown",
		"vm address space shortage",
		"unknown",
		"unknown",
		"unknown",
		"ipc namespace shortage",
	};

	if (argc < 2)
		return retval;

	const char *arg = argv[1];
	unsigned long code = 0;
	int type = 0;

	if (strcmp(arg, "mach") == 0) {
		if (argc < 3)
			return retval;

		code = strtoul(argv[2], NULL, 0);
		type = 1;
	} else if (strcmp(arg, "bootstrap") == 0) {
		if (argc < 3)
			return retval;

		code = strtoul(argv[2], NULL, 0);
		type = 2;
	} else if (strcmp(arg, "posix") == 0) {
		if (argc < 3)
			return retval;

		code = strtoul(argv[2], NULL, 0);
		type = 3;
	} else {
		code = strtoul(arg, NULL, 0);
		if (code < 161)
			type = 3;
		else {
			const char *other = "0x";
			if (strncmp(arg, other, strlen(other)) == 0) {
				type = 1;
			} else {
				type = 2;
			}
		}
	}

	if (type == 1) {
		unsigned long extraCode = code & 0x3E00;
		char *str = mach_error_string((mach_error_t)(code & 0xFFFFC1FF));

		fprintf(stdout, "0x%lx: %s\n", code, str);
		fprintf(stdout, "system = 0x%x\n", (unsigned int)err_get_system(code));
		fprintf(stdout, "subsystem = 0x%x\n", (unsigned int)err_get_sub(code));
		fprintf(stdout, "code = 0x%x\n", (unsigned int)(code & 0x1FF));

		if (extraCode != 0) {
			unsigned long extraCodeIndex = (extraCode - 1024) >> 10;
			const char *extraStr = NULL;
			if (extraCodeIndex <= 7)
				extraStr = extraMessagesTable[extraCodeIndex];
			else
				extraStr = "unknown";

			fprintf(stdout, "extra = 0x%x: %s\n", (unsigned int)extraCode, extraStr);
		}

		retval = 0;
	} else if (type == 2) {
		const char *str = bootstrap_strerror((kern_return_t)code);
		fprintf(stdout, "%u: %s\n", (unsigned int)code, str);
		retval = 0;
	} else if (type == 3) {
		const char *str = xpc_strerror((int)code);
		fprintf(stdout, "%u: %s\n", (unsigned int)code, str);
		retval = 0;
	} else {
		retval = 0;
	}

	return retval;
}
