/*-
 * SPDX-License-Identifier: BSD-2-Clause
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
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

static int64_t
limit_index(const char *name)
{
	const char *limitnames[9] = { "cpu", "filesize", "data", "stack", "core", "rss", "memlock", "maxproc",
		"maxfiles" };
	for (unsigned long i = 0; i < (sizeof(limitnames) / sizeof(limitnames[0])); i++) {
		if (strcmp(name, limitnames[i]) == 0) {
			return i;
		}
	}
	return -1;
}

static long long
parse_limit(const char *name)
{
	long long num;
	char *end;

	if (strcmp(name, "unlimited") == 0) {
		errno = 0;
		return -1;
	}

	num = strtoll(name, &end, 0);

	if (end[0] != '\0') {
		errno = EINVAL;
		return -1;
	}

	return num;
}

int
limit_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc > 4)
		return EUSAGE;

	xpc_object_t dict, reply;
	int err = 0;
	vm_address_t addr;
	vm_size_t sz = 0x100000;
	long long softlimit, hardlimit, idx;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	launchctl_setup_xpc_dict_for_service_name("system", dict, NULL);

	if (argc < 2) {
	printlimits:
		xpc_dictionary_set_bool(dict, "print", true);

		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			addr = launchctl_create_shmem(dict, sz);
		} else {
			xpc_dictionary_set_fd(dict, "file", STDOUT_FILENO);
		}

		err = launchctl_send_xpc_to_launchd(XPC_ROUTINE_LIMIT, dict, &reply);
		if (err != 0) {
			fprintf(stderr, "Could not print resource limits: %d: %s\n", err, xpc_strerror(err));
		}

		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else {
		if ((idx = limit_index(argv[1])) == -1) {
			fprintf(stderr, "%s is not a valid limit name.\n", argv[1]);
			return EUSAGE;
		}

		xpc_dictionary_set_int64(dict, "which", idx);

		if (argc < 3) {
			goto printlimits;
		}

		softlimit = parse_limit(argv[2]);
		if (softlimit == -1 && errno != 0) {
			fprintf(stderr, "%s is not a valid limit.\n", argv[2]);
			return EUSAGE;
		}
		hardlimit = softlimit;

		if (argc > 3) {
			hardlimit = parse_limit(argv[3]);
			if (hardlimit == -1 && errno != 0) {
				fprintf(stderr, "%s is not a valid limit.\n", argv[3]);
				return EUSAGE;
			}
		}

		xpc_dictionary_set_int64(dict, "softlimit", softlimit);
		xpc_dictionary_set_int64(dict, "hardlimit", hardlimit);

		err = launchctl_send_xpc_to_launchd(XPC_ROUTINE_LIMIT, dict, &reply);
		if (err != 0) {
			fprintf(stderr, "Could not set resource limits: %d: %s\n", err, xpc_strerror(err));
		}
	}

	return err;
}
