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
#include <mach/mach.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
print_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;
	int ret = EUSAGE;
	xpc_object_t reply;
	const char *name = NULL;
	vm_address_t addr;
	vm_size_t sz = 0x100000;

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	if ((ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name)) != 0)
		return ret;

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}

	if (name != NULL)
		ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT_SERVICE, dict, &reply);
	else
		ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret == 0) {
		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else if (ret < ENODOMAIN) {
		if (ret == EINVAL)
			fprintf(stderr, "Bad request.\n");
		else
			fprintf(stderr, "Could not print domain: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}

int
print_cache_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	xpc_object_t reply;
	vm_address_t addr;
	vm_size_t sz = 0x1400000;

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	xpc_dictionary_set_uint64(dict, "type", 1);
	xpc_dictionary_set_uint64(dict, "handle", 0);

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}
	xpc_dictionary_set_bool(dict, "cache", true);

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret == 0) {
		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else if (ret < ENODOMAIN) {
		if (ret == EINVAL)
			fprintf(stderr, "Bad request.\n");
		else
			fprintf(stderr, "Could not print cache: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}

int
print_disabled_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	xpc_object_t reply;
	vm_address_t addr;
	vm_size_t sz = 0x100000;

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	xpc_dictionary_set_uint64(dict, "type", 1);
	xpc_dictionary_set_uint64(dict, "handle", 0);

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}
	xpc_dictionary_set_bool(dict, "disabled", true);

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT, dict, &reply);

	if (ret == 0) {
		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else if (ret < ENODOMAIN) {
		if (ret == EINVAL)
			fprintf(stderr, "Bad request.\n");
		else
			fprintf(stderr, "Could not print cache: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}

int
dumpstate_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	xpc_object_t reply;
	vm_address_t addr;
	vm_size_t sz = 0x1400000;

	xpc_object_t dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	xpc_dictionary_set_uint64(dict, "type", 1);
	xpc_dictionary_set_uint64(dict, "handle", 0);

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_DUMPSTATE, dict, &reply);

	if (ret == 0) {
		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else if (ret == EBUSY) {
		fprintf(stderr, "State-dump already in progress; please try again later.\n");
	} else if (ret == ENOTSUP) {
		fprintf(stderr, "State-dump is not available in this configuration.\n");
	} else if (ret == ENOTDEVELOPMENT) {
		fprintf(stderr, "State-dump is only available on the DEVELOPMENT variant.\n");
	} else {
		fprintf(stderr, "State-dump failed with error %d\n", ret);
	}

	return ret;
}
