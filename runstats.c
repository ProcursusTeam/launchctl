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
#include <sys/resource.h>

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

static void
print_runstats(size_t index, xpc_object_t dict, void *ctx)
{
	size_t ru_len = 0;
	const struct rusage *ru = (const struct rusage *)xpc_dictionary_get_data(dict, "rusage", &ru_len);
	if (ru_len != sizeof(struct rusage)) {
		fprintf(stderr, "runstats ipc routine returned incorrectly sized struct rusage\n");
		exit(1);
	}
	int64_t pid = xpc_dictionary_get_int64(dict, "pid");
	int64_t reason = xpc_dictionary_get_int64(dict, "run-reason");
	uint64_t start = xpc_dictionary_get_uint64(dict, "start");
	uint64_t end = xpc_dictionary_get_uint64(dict, "end");
	uint64_t forks = xpc_dictionary_get_uint64(dict, "forks");
	uint64_t execs = xpc_dictionary_get_uint64(dict, "execs");
	bool dirty_exit = xpc_dictionary_get_bool(dict, "dirty-exit");
	bool idle_exit = xpc_dictionary_get_bool(dict, "idle-exit");
	bool jettisoned = xpc_dictionary_get_bool(dict, "jettisoned");
	printf("run %lu = {\n", index);
	printf("\tpid = %lld\n", pid);
	printf("\treason = %lld\n", reason);
	printf("\tstart = %llu\n", start);
	printf("\tend = %llu\n", end);
	printf("\tforks = %llu\n", forks);
	printf("\texecs = %llu\n", execs);
	printf("\tdirty exit = %d\n", dirty_exit);
	printf("\tidle exit = %d\n", idle_exit);
	printf("\tjettisoned = %d\n", jettisoned);
	printf("\trusage = {\n");
	printf("\t\tuser time = %ld\n", ru->ru_utime.tv_sec);
	printf("\t\tsystem time = %ld\n", ru->ru_stime.tv_sec);
	printf("\t\tmax resident set = %ld\n", ru->ru_maxrss);
	printf("\t\tintegral shared memory size = %ld\n", ru->ru_ixrss);
	printf("\t\tintegral unshared data size = %ld\n", ru->ru_idrss);
	printf("\t\tintegral unshared stack size = %ld\n", ru->ru_isrss);
	printf("\t\tpage reclaims = %ld\n", ru->ru_minflt);
	printf("\t\tpage faults = %ld\n", ru->ru_majflt);
	printf("\t\tswaps = %ld\n", ru->ru_nswap);
	printf("\t\tblock input operations = %ld\n", ru->ru_inblock);
	printf("\t\tblock input operations = %ld\n", ru->ru_inblock);
	printf("\t\tblock output operations = %ld\n", ru->ru_oublock);
	printf("\t\tmessages sent = %ld\n", ru->ru_msgsnd);
	printf("\t\tmessages received = %ld\n", ru->ru_msgrcv);
	printf("\t\tsignals received = %ld\n", ru->ru_nsignals);
	printf("\t\tvoluntary context switches = %ld\n", ru->ru_nvcsw);
	printf("\t\tinvoluntary context switches = %ld\n", ru->ru_nivcsw);
	printf("\t}\n");
	printf("}\n");
	return;
}

int
runstats_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc != 2)
		return EUSAGE;

	xpc_object_t dict, reply;
	const char *name = NULL;
	xpc_object_t runs;
	int ret;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;

	if ((ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name))) {
		return ret;
	}
	if (name == NULL)
		return EBADNAME;

	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_RUNSTATS, dict, &reply);
	if (ret == ENOTSUP) {
		fprintf(stderr, "Performance logging is not enabled.\n");
		return ret;
	} else if (ret == ENOENT) {
		fprintf(stderr, "No resource statistics gathered for service yet.\n");
		return ret;
	} else if (ret == EINVAL) {
		fprintf(stderr, "Bad Request.\n");
		return ret;
	} else if (ret) {
		return ret;
	}

	ret = 0;
	runs = xpc_dictionary_get_value(reply, "runs");
	if (runs == NULL || xpc_get_type(runs) != XPC_TYPE_ARRAY) {
		return EINVAL;
	}

	printf("\"%s\"\n", name);
	xpc_array_apply_f(runs, NULL, print_runstats);
	return ret;
}
