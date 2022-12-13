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
#include <stdio.h>
#include <sysdir.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int
bootstrap_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	const char *name = NULL;
	xpc_object_t dict, reply, paths;

	if (argc < 2)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name);
	if (ret != 0)
		return ret;
	if (name != NULL)
		return EBADNAME;

	if (argc > 2) {
		paths = launchctl_parse_load_unload(0, argc - 2, argv + 2);
		xpc_dictionary_set_value(dict, "paths", paths);
	}
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_LOAD, dict, &reply);
	if (ret != ENODOMAIN) {
		if (ret == 0) {
			xpc_object_t errors;
			errors = xpc_dictionary_get_value(reply, "errors");
			if (errors == NULL || xpc_get_type(errors) != XPC_TYPE_DICTIONARY)
				return 0;
			(void)xpc_dictionary_apply(errors, ^bool(const char *key, xpc_object_t value) {
					if (xpc_get_type(value) == XPC_TYPE_INT64) {
						int64_t err = xpc_int64_get_value(value);
						if (err == EEXIST || err == EALREADY)
							fprintf(stderr, "%s: service already loaded\n", key);
						else
							fprintf(stderr, "%s: %s\n", key, xpc_strerror(err));
					}
					return true;
			});
			if (xpc_dictionary_get_count(errors) != 0)
				return EMANY;
			else
				return 0;
		}
		fprintf(stderr, "Bootstrap failed: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}

int
bootout_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int ret;
	const char *name = NULL;
	xpc_object_t dict, reply, paths;

	if (argc < 2)
		return EUSAGE;

	dict = xpc_dictionary_create(NULL, NULL, 0);
	*msg = dict;
	ret = launchctl_setup_xpc_dict_for_service_name(argv[1], dict, &name);
	if (ret != 0)
		return ret;

	if (argc > 2 && name == NULL) {
		paths = launchctl_parse_load_unload(0, argc - 2, argv + 2);
		xpc_dictionary_set_value(dict, "paths", paths);
	}
	ret = launchctl_send_xpc_to_launchd(XPC_ROUTINE_UNLOAD, dict, &reply);
	if (ret != ENODOMAIN) {
		if (ret == 0) {
			xpc_object_t errors;
			errors = xpc_dictionary_get_value(reply, "errors");
			if (errors == NULL || xpc_get_type(errors) != XPC_TYPE_DICTIONARY)
				return 0;
			(void)xpc_dictionary_apply(errors, ^bool(const char *key, xpc_object_t value) {
					if (xpc_get_type(value) == XPC_TYPE_INT64) {
						int64_t err = xpc_int64_get_value(value);
						if (err == EEXIST || err == EALREADY)
							fprintf(stderr, "%s: service already loaded\n", key);
						else
							fprintf(stderr, "%s: %s\n", key, xpc_strerror(err));
					}
					return true;
			});
			if (xpc_dictionary_get_count(errors) != 0)
				return EMANY;
			else
				return 0;
		}
		fprintf(stderr, "Boot-out failed: %d: %s\n", ret, xpc_strerror(ret));
	}

	return ret;
}
