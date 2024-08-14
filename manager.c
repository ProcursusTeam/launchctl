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
#include <inttypes.h>
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "vproc_priv.h"
#include "xpc_private.h"

int
managerpid_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	void *err;
	int64_t pid;

	err = vproc_swap_integer(NULL, VPROC_GSK_MGR_PID, 0, &pid);
	if (err == NULL)
		printf("%" PRId64 "\n", pid);
	else {
		fprintf(stderr, "Could not get manager PID.\n");
		return EWTF;
	}

	return 0;
}

int
manageruid_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	void *err;
	int64_t uid;

	err = vproc_swap_integer(NULL, VPROC_GSK_MGR_UID, 0, &uid);
	if (err == NULL)
		printf("%" PRId64 "\n", uid);
	else {
		fprintf(stderr, "Could not get manager UID.\n");
		return EWTF;
	}

	return 0;
}

int
managername_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	void *err;
	char *name;

	err = vproc_swap_string(NULL, VPROC_GSK_MGR_NAME, NULL, &name);
	if (err == NULL)
		printf("%s\n", name);
	else {
		fprintf(stderr, "Could not get manager name.\n");
		return EWTF;
	}

	return 0;
}
