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
#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>
typedef xpc_object_t xpc_pipe_t;

#include <Availability.h>

#define DYLD_INTERPOSE(_replacement,_replacee) \
   __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
            __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };

kern_return_t
xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t XPC_GIVES_REFERENCE *reply);

kern_return_t
hook_xpc_pipe_routine(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t XPC_GIVES_REFERENCE *reply)
{
	kern_return_t ret = xpc_pipe_routine(pipe, request, reply);
	char *requeststr = xpc_copy_description(request);
	fprintf(stderr, "\033[32mREQUEST: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_routine, xpc_pipe_routine);

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 130000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 101500
kern_return_t
xpc_pipe_routine_with_flags(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t XPC_GIVES_REFERENCE *reply, uint64_t flags);

kern_return_t
hook_xpc_pipe_routine_with_flags(xpc_pipe_t pipe, xpc_object_t request,
	xpc_object_t XPC_GIVES_REFERENCE *reply, uint64_t flags)
{
	kern_return_t ret = xpc_pipe_routine_with_flags(pipe, request, reply, flags);
	char *requeststr = xpc_copy_description(request);
	fprintf(stderr, "\033[32mREQUEST: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	fprintf(stderr, "\033[32mFLAGS: %llu\033[m\n", flags);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_routine_with_flags, xpc_pipe_routine_with_flags);
#endif

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 150000 || __MAC_OS_X_VERSION_MIN_REQUIRED >= 120000
int _xpc_pipe_interface_routine(xpc_pipe_t pipe, uint64_t routine, xpc_object_t msg, xpc_object_t XPC_GIVES_REFERENCE *reply, uint64_t flags);

int
hook_xpc_pipe_interface_routine(xpc_pipe_t pipe, uint64_t routine, xpc_object_t msg, xpc_object_t XPC_GIVES_REFERENCE *reply, uint64_t flags)
{
	int ret = _xpc_pipe_interface_routine(pipe, routine, msg, reply, flags);
	fprintf(stderr, "\033[32mROUTINE: %llu\033[m\n", routine);
	char *requeststr= xpc_copy_description(msg);
	fprintf(stderr, "\033[32mMSG: %s\033[m\n", requeststr);
	free(requeststr);
	char *replystr = xpc_copy_description(*reply);
	fprintf(stderr, "\033[31mREPLY: %s\033[m\n", replystr);
	free(replystr);
	fprintf(stderr, "\033[32mFLAGS: %llu\033[m\n", flags);
	return ret;
}

DYLD_INTERPOSE(hook_xpc_pipe_interface_routine, _xpc_pipe_interface_routine);
#endif
