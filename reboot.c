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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

int reboot3(uint64_t flags, ...);

#define RB2_USERREBOOT (0x2000000000000000llu)
#define RB2_OBLITERATE (0x4000000000000000llu)
#define RB2_FULLREBOOT (0x8000000000000000llu)
#define ITHINK_HALT (0x8000000000000008llu)

int
reboot_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	uint64_t flags = RB2_FULLREBOOT;
	if (argc < 2 || strcmp(argv[1], "system") == 0)
		flags = RB2_FULLREBOOT;
	else if (strcmp(argv[1], "halt") == 0)
		flags = ITHINK_HALT;
	else if (strcmp(argv[1], "userspace") == 0)
		flags = RB2_USERREBOOT;
	else if (strcmp(argv[1], "obliterate") == 0)
		flags = RB2_OBLITERATE;

	// TODO: Add -s, I don't care enough to add it now
	// TODO: Test this lol

	ret = reboot3(flags);
	if (ret != 0) {
		fprintf(stderr, "Failed to %s: %d: %s\n", argv[1], ret, xpc_strerror(ret));
	}
	return ret;
}
