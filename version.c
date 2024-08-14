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
#include <stdio.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
version_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	xpc_object_t reply;
	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	launchctl_setup_xpc_dict(dict);
	vm_address_t addr = 0;
	vm_size_t sz = 0x100000;

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}

	if (strcmp(argv[0], "variant") == 0)
		xpc_dictionary_set_bool(dict, "variant", 1);
	else
		xpc_dictionary_set_bool(dict, "version", 1);

	int ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret == EINVAL) {
		fprintf(stderr, "Bad request.\n");
	} else if (ret != 0) {
		fprintf(stderr, "Could not print variant: %d: %s\n", ret, xpc_strerror(ret));
	}

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		launchctl_print_shmem(reply, addr, sz, stdout);

		if (addr != 0)
			vm_deallocate(mach_task_self(), addr, sz);
	}

	return ret;
}
