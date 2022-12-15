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
#include <xpc/xpc.h>

typedef int cmd_main(xpc_object_t *, int, char **, char **, char **);

// launchctl.c
cmd_main list_cmd;
cmd_main version_cmd;
cmd_main help_cmd;

// start_stop.c
cmd_main stop_cmd;
cmd_main start_cmd;

// print.c
cmd_main print_cmd;

// env.c
cmd_main setenv_cmd;
cmd_main getenv_cmd;

// load.c
cmd_main load_cmd;

// enable.c
cmd_main enable_cmd;

// reboot.c
cmd_main reboot_cmd;

// bootstrap.c
cmd_main bootstrap_cmd;
cmd_main bootout_cmd;

// error.c
cmd_main error_cmd;

void launchctl_xpc_object_print(xpc_object_t, const char *name, int level);
int launchctl_send_xpc_to_launchd(uint64_t routine, xpc_object_t msg, xpc_object_t *reply);
void launchctl_setup_xpc_dict(xpc_object_t dict);
int launchctl_setup_xpc_dict_for_service_name(char *servicetarget, xpc_object_t dict, const char **name);
void launchctl_print_domain_str(FILE *s, xpc_object_t msg);
xpc_object_t launchctl_parse_load_unload(unsigned int domain, int count, char **list);
vm_address_t launchctl_create_shmem(xpc_object_t, vm_size_t);
void launchctl_print_shmem(xpc_object_t dict, vm_address_t addr, vm_size_t sz, FILE *outfd);

// This is part of compiler-rt, I just don't want to use objc
int32_t __isPlatformVersionAtLeast(uint32_t Platform, uint32_t Major, uint32_t Minor, uint32_t Subminor);
