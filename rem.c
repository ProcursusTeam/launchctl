/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Procursus Team <team@procurs.us>
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int
enter_rem_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret = ENOTSUP;

	if (__builtin_available(macOS 15.0, iOS 18.0, tvOS 18.0, watchOS 11.0, bridgeOS 9.0, *))
		ret = launch_userspace_reboot_with_purpose(4);

	if (ret != 0)
		fprintf(stderr, "Failed to enter REM: %d: %s\n", ret, xpc_strerror(ret));

	return ret;
}

int
enter_rem_dev_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret = ENOTSUP;

	if (__builtin_available(macOS 15.0, iOS 18.0, tvOS 18.0, watchOS 11.0, bridgeOS 9.0, *))
		ret = launch_userspace_reboot_with_purpose(5);

	if (ret != 0)
		fprintf(stderr, "Failed to enter REM (development): %d: %s\n", ret, xpc_strerror(ret));

	return ret;
}
