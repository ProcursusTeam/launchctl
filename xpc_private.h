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
#include <xpc/xpc.h>

#ifndef _XPC_PRIVATE_H_
#define _XPC_PRIVATE_H_

enum {
	XPC_ROUTINE_PRINT_SERVICE = 708,
	XPC_ROUTINE_LOAD = 800,
	XPC_ROUTINE_UNLOAD = 801,
	XPC_ROUTINE_ENABLE = 808,
	XPC_ROUTINE_DISABLE = 809,
	XPC_ROUTINE_SERVICE_KILL = 812,
	XPC_ROUTINE_SERVICE_START = 813,
	XPC_ROUTINE_SERVICE_STOP = 814,
	XPC_ROUTINE_LIST = 815,
	XPC_ROUTINE_REMOVE = 816,
	XPC_ROUTINE_SETENV = 819,
	XPC_ROUTINE_GETENV = 820,
	XPC_ROUTINE_PRINT = 828,
	XPC_ROUTINE_DUMPSTATE = 834,
};

typedef xpc_object_t xpc_pipe_t;

int xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t message, xpc_object_t XPC_GIVES_REFERENCE *reply);
int _xpc_pipe_interface_routine(xpc_pipe_t pipe, uint64_t routine, xpc_object_t message, xpc_object_t XPC_GIVES_REFERENCE *reply, uint64_t flags);

const char *xpc_strerror(int);

#define XPC_TYPE_MACH_SEND (&_xpc_type_mach_send)
XPC_EXPORT
XPC_TYPE(_xpc_type_mach_send);

typedef void (*xpc_dictionary_applier_f)(const char *key, xpc_object_t val, void *ctx);
void xpc_dictionary_apply_f(xpc_object_t xdict, void *ctx, xpc_dictionary_applier_f applier);

enum {
	ENODOMAIN = 112,
	ENOSERVICE,
	E2BIMPL = 116,
	EUSAGE,
	EBADRESP,
	EMANY = 133,
	EBADNAME = 140,
	ENOTDEVELOPMENT = 142,
};

#endif
