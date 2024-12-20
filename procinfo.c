/*-
 * SPDX-License-Identifier: BSD 2-Clause License
 *
 * Copyright (c) 2024 Procursus Team <team@procurs.us>
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
#define __APPLE_API_UNSTABLE

#include <sys/sysctl.h>

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <libproc.h>
#include <mach/mach_traps.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

/* #include <mach/port_descriptions.h> */
const char *mach_host_special_port_description(int port);
const char *mach_task_special_port_description(int port);
/* END <mach/mach/port_descriptions.h> */

#include <sys/proc_info.h>

/* #define PRIVATE
   #include <sys/proc_info_private.h> */
struct proc_uniqidentifierinfo {
	uint8_t p_uuid[16];   /* UUID of the main executable */
	uint64_t p_uniqueid;  /* 64 bit unique identifier for process */
	uint64_t p_puniqueid; /* unique identifier for process's parent */
	int32_t p_idversion;  /* pid version */
	uint32_t p_reserve2;  /* reserved for future use */
	uint64_t p_reserve3;  /* reserved for future use */
	uint64_t p_reserve4;  /* reserved for future use */
};

struct proc_bsdinfowithuniqid {
	struct proc_bsdinfo pbsd;
	struct proc_uniqidentifierinfo p_uniqidentifier;
};

#define PROC_PIDT_BSDINFOWITHUNIQID 18
#define PROC_PIDT_BSDINFOWITHUNIQID_SIZE (sizeof(struct proc_bsdinfowithuniqid))
/* END <sys/proc_info_private.h> */

/* #include <sys/codesign.h> */
#define CS_OPS_STATUS 0			/* return status */
#define CS_OPS_ENTITLEMENTS_BLOB 7	/* get entitlements blob */
#define CS_OPS_DER_ENTITLEMENTS_BLOB 16 /* get der entitlements blob */

int csops(pid_t pid, unsigned int ops, void *useraddr, size_t usersize);
int csops_audittoken(pid_t pid, unsigned int ops, void *useraddr, size_t usersize, audit_token_t *token);
/* END <sys/codesign.h> */

/* #include <System/kern/cs_blobs.h> */
typedef struct __SC_GenericBlob {
	uint32_t magic;	 /* magic number */
	uint32_t length; /* total length of blob */
	char data[];
} CS_GenericBlob __attribute__((aligned(1)));

/* code signing attributes of a process */
#define CS_VALID 0x00000001	     /* dynamically valid */
#define CS_ADHOC 0x00000002	     /* ad hoc signed */
#define CS_GET_TASK_ALLOW 0x00000004 /* has get-task-allow entitlement */
#define CS_INSTALLER 0x00000008	     /* has installer entitlement */

#define CS_HARD 0x00000100	       /* don't load invalid pages */
#define CS_KILL 0x00000200	       /* kill process if it becomes invalid */
#define CS_CHECK_EXPIRATION 0x00000400 /* force expiration checking */
#define CS_RESTRICT 0x00000800	       /* tell dyld to treat restricted */

#define CS_ENFORCEMENT 0x00001000	     /* require enforcement */
#define CS_REQUIRE_LV 0x00002000	     /* require library validation */
#define CS_ENTITLEMENTS_VALIDATED 0x00004000 /* code signature permits restricted entitlements */

#define CS_RUNTIME 0x00010000	    /* Apply hardened runtime policies */
#define CS_LINKER_SIGNED 0x00020000 /* Automatically signed by the linker */

#define CS_ALLOWED_MACHO                                                                                     \
	(CS_ADHOC | CS_HARD | CS_KILL | CS_CHECK_EXPIRATION | CS_RESTRICT | CS_ENFORCEMENT | CS_REQUIRE_LV | \
	    CS_RUNTIME | CS_LINKER_SIGNED)
#define CS_NO_UNTRUSTED_HELPERS 0x02000000	 /* kernel did not load a non-platform-binary dyld or Rosetta runtime */
#define CS_DYLD_PLATFORM CS_NO_UNTRUSTED_HELPERS /* old name */
#define CS_PLATFORM_BINARY 0x04000000		 /* this is a platform binary */

#define CSMAGIC_EMBEDDED_ENTITLEMENTS 0xfade7171
#define CSMAGIC_EMBEDDED_DER_ENTITLEMENTS = 0xfade7172
/* END <System/kern/cs_blobs.h> */

static char *
safe_strdup(const char *str)
{
	char *retval = NULL;
	while ((retval = strdup(str)) == NULL) {
		assert(errno == ENOMEM);
		sleep(1);
	}
	return retval;
}

static void *
safe_calloc(size_t size)
{
	void *retval = NULL;
	while ((retval = calloc(1, size)) == NULL) {
		assert(errno == ENOMEM);
		sleep(1);
	}
	return retval;
}

static char *
get_port_type(mach_port_t port)
{
	const char *type = NULL;
	xpc_object_t dict = NULL, reply = NULL;
	if (port == mach_host_self()) {
		type = "host";
		goto port_type_end;
	}

	dict = xpc_dictionary_create(NULL, NULL, 0);
	launchctl_setup_xpc_dict_for_service_name("system", dict, NULL);
	xpc_dictionary_set_int64(dict, "process", getpid());
	xpc_dictionary_set_uint64(dict, "name", port);
	int retval = launchctl_send_xpc_to_launchd(XPC_ROUTINE_RESOLVE_PORT, dict, &reply);
	if (retval) {
		type = "unknown";
		goto port_type_end;
	}

	type = xpc_dictionary_get_string(reply, "endpoint");
	if (type)
		goto port_type_end;
	type = xpc_dictionary_get_string(reply, "domain");
	if (!type)
		type = "unknown";

port_type_end:
	if (reply)
		xpc_release(reply);
	if (dict)
		xpc_release(dict);
	return safe_strdup(type);
}

// hostinfo: isproc == 0, procinfo: isproc == 1
/*
 * This functions CONSUMES the old_behaviors!
*/
static void
print_exception_port_info(int64_t isproc, exception_mask_array_t masks, mach_msg_type_number_t masksCnt,
    exception_handler_array_t old_handlers)
{
	if (masksCnt == 0)
		return;
	for (mach_msg_type_number_t i = 0; i < masksCnt; i++) {
		if (!MACH_PORT_VALID(old_handlers[i]))
			continue;
		char *info = get_port_type(old_handlers[i]);
		if (isproc) {
			printf("\t");
		}
		if (info[0] == '(') {
			printf("exception port = 0x%x %s\n", old_handlers[i], info);
		} else {
			printf("exception port = 0x%x (%s)\n", old_handlers[i], info);
		}
		free(info);
		exception_mask_t mask = masks[i];
		if ((mask >> EXC_BAD_ACCESS) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_BAD_ACCESS\n");
		}
		if ((mask >> EXC_BAD_INSTRUCTION) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_BAD_INSTRUCTION\n");
		}
		if ((mask >> EXC_ARITHMETIC) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_ARITHMETIC\n");
		}
		if ((mask >> EXC_EMULATION) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_EMULATION\n");
		}
		if ((mask >> EXC_SOFTWARE) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_SOFTWARE\n");
		}
		if ((mask >> EXC_BREAKPOINT) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_BREAKPOINT\n");
		}
		if ((mask >> EXC_SYSCALL) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_SYSCALL\n");
		}
		if ((mask >> EXC_MACH_SYSCALL) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_MACH_SYSCALL\n");
		}
		if ((mask >> EXC_RPC_ALERT) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_RPC_ALERT\n");
		}
		if ((mask >> EXC_CRASH) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_CRASH\n");
		}
		if ((mask >> EXC_RESOURCE) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_RESOURCE\n");
		}
		if ((mask >> EXC_GUARD) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_GUARD\n");
		}
		if ((mask >> EXC_CORPSE_NOTIFY) & 1) {
			if (isproc)
				printf("\t");
			printf("\tEXC_CORPSE_NOTIFY\n");
		}
		mach_port_deallocate(mach_task_self(), old_handlers[i]);
	}
}

static int
csops_and_flip_endian(pid_t pid, audit_token_t *token, int op, CS_GenericBlob *buf, size_t bufsz)
{
	int retval;
	if (token) {
		retval = csops_audittoken(pid, op, buf, bufsz, token);
	} else {
		retval = csops(pid, op, buf, bufsz);
	}
	buf->magic = ntohl(buf->magic);
	buf->length = ntohl(buf->length);
	return retval;
}

static xpc_object_t
get_entitlements_internal(pid_t pid, audit_token_t *token, int op)
{
	CS_GenericBlob *buf = __builtin_alloca(0x408);
	CS_GenericBlob *buf2 = NULL;
	xpc_object_t xdata = NULL;
	buf->magic = 0;
	buf->length = 0;
	memset(&buf->data, 0xaa, 0x400);
	if (op != CS_OPS_ENTITLEMENTS_BLOB && op != CS_OPS_DER_ENTITLEMENTS_BLOB)
		return NULL;
	int retval = csops_and_flip_endian(pid, token, op, buf, 0x408);
	if (retval < 0) {
		if (errno != ERANGE)
			return NULL;
		size_t bufsz = (size_t)buf->length;
		buf2 = safe_calloc(bufsz);
		buf = buf2;
		retval = csops_and_flip_endian(pid, token, op, buf, bufsz);
		if (retval) {
			free(buf2);
			return NULL;
		}
	}
	if (buf->length != 0 && buf->magic != 0) {
		xdata = xpc_data_create(buf->data, (size_t)(buf->length - 8));
	}
	if (buf2)
		free(buf2);
	return xdata;
}

xpc_object_t
get_entitlements(pid_t pid, audit_token_t *token)
{
	xpc_object_t xdict = NULL;
	xpc_object_t xdata = get_entitlements_internal(pid, token, CS_OPS_ENTITLEMENTS_BLOB);
	if (xdata) {
		xdict = xpc_create_from_plist(xpc_data_get_bytes_ptr(xdata), xpc_data_get_length(xdata));
		xpc_release(xdata);
	}
	return xdict;
}

int
procinfo_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	if (argc < 2)
		return EUSAGE;

	pid_t pid = strtol(argv[1], NULL, 0);

	if (!pid)
		return EUSAGE;
	if (pid == -1)
		pid = getpid();

	char path[PATH_MAX];
	memset(path, 0xaa, PATH_MAX);
	int retval = proc_pidpath(pid, path, PATH_MAX);

	if (retval < 1) {
		printf("program path = (could not resolve path)\n");
	} else {
		printf("program path = %s\n", path);
	}

	mach_port_t task = 0;
	kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);

	if (kr != 0) {
		// Yes, this goes to stdout
		printf("Could not print Mach info for pid %d: 0x%x\n", pid, kr);
		goto procinfo_proc_info;
	}

	exception_mask_t masks[EXC_TYPES_COUNT];
	mach_msg_type_number_t masksCnt;
	exception_handler_t old_handlers[EXC_TYPES_COUNT];
	exception_behavior_t old_behaviors[EXC_TYPES_COUNT];
	thread_state_flavor_t old_flavors[EXC_TYPES_COUNT];

	kr = task_get_exception_ports(task, 0x3ffe, masks, &masksCnt, old_handlers, old_behaviors, old_flavors);

	if (kr != 0) {
		// this goes to stdout as well
		printf("Could not print Mach exception info for pid %d: 0x%x\n", pid, kr);
		goto procinfo_proc_info;
	}

	printf("mach info = {\n");
	for (int i = 0; i < TASK_MAX_SPECIAL_PORT; i++) {
		mach_port_t port;
		kr = task_get_special_port(task, i, &port);
		if (kr != 0)
			continue;
		char *port_info = get_port_type(port);
		const char *port_description = mach_task_special_port_description(i);
		if (port_info[0] == '(') {
			printf("\ttask-%s port = 0x%x %s\n", port_description, port, port_info);
		} else {
			printf("\ttask-%s port = 0x%x (%s)\n", port_description, port, port_info);
		}
		kr = mach_port_deallocate(mach_task_self(), port);
		free(port_info);
	}
	print_exception_port_info(1, masks, masksCnt, old_handlers);
	printf("}\n");

procinfo_proc_info : {
}
	if (task)
		mach_port_deallocate(mach_task_self(), task);

	struct proc_bsdinfowithuniqid procinfo;
	retval = proc_pidinfo(pid, PROC_PIDT_BSDINFOWITHUNIQID, 1, &procinfo, sizeof(struct proc_bsdinfowithuniqid));
	char *procargs = NULL; // declare here because of goto

	if (retval != sizeof(struct proc_bsdinfowithuniqid)) {
		fprintf(stderr, "Could not get proc info PID %d: %d: %s\n", pid, errno, strerror(errno));
		goto procinfo_pressured_exit_info;
	}

	int argmax_mib[2] = { CTL_KERN, KERN_ARGMAX };
	int32_t argmax;
	size_t argmax_size = sizeof(int32_t);
	retval = sysctl(argmax_mib, 2, &argmax, &argmax_size, NULL, 0);

	if (retval) {
		fprintf(stderr, "Could not get max argument size: %d: %s\n", errno, strerror(errno));
		goto procinfo_pressured_exit_info;
	}

	procargs = safe_calloc((size_t)argmax);
	size_t argmax2 = (size_t)argmax;
	int procargs_mib[3] = { CTL_KERN, KERN_PROCARGS2, pid };
	// adv_cmds/ps/print.c
	retval = sysctl(procargs_mib, 3, procargs, &argmax2, NULL, 0);

	if (retval) {
		fprintf(stderr, "Could not get process arguments: %d: %s\n", errno, strerror(errno));
		goto procinfo_pressured_exit_info;
	}

	int proc_argc = *(int *)procargs;

	if (proc_argc < 0) {
		fprintf(stderr, "Process had negative number of arguments. Kernel bug?\n");
		goto procinfo_pressured_exit_info;
	}

	printf("argument count = %d\n", proc_argc);

	char *p = procargs;
	for (p += 4; *p != '\0' && p < &procargs[argmax]; p++)
		; /* skip args and exec_path */
	assert(p < &procargs[argmax]);

	for (; *p == '\0' && p < &procargs[argmax]; p++)
		; /* skip padding */
	assert(p < &procargs[argmax]);

	// reached argv[0]
	char *proc_argv = (char *)p;
	printf("argument vector = {\n");
	for (int32_t i = 0; i < proc_argc && p < &procargs[argmax]; i++) {
		printf("\t[%d] = %s\n", i, p);
		p += strlen(p);
		for (; *p == '\0' && p < &procargs[argmax]; p++)
			; /* skip padding */
	}
	printf("}\n");

	// Bug-to-bug compatibility: If process has no environment at all Apple's
	// launchctl will print the first element in apple (ptr_munge) so we do that too.
	for (; *p == '\0' && p < &procargs[argmax]; p++)
		; /* skip padding (or nonexistent environment) */

	// Now print environment (or ptr_munge)
	printf("environment vector = {\n");
	while (p < &procargs[argmax] && *p != '\0') {
		char *eq = strrchr(p, '=');
		if (!eq) {
			printf("\t%s (malformed)\n", p);
		} else {
			*eq = '\0';
			printf("\t%s => %s\n", p, eq + 1);
			*eq = '=';
		}
		p += strlen(p);
		p++;
	}

	printf("}\n");

	static const char *const status_strs[] = { "running", "stopped", "lost to control", "zombie",
		"terminated (with core)", "idle" };
	const char *status_str = procinfo.pbsd.pbi_status < 7 ? status_strs[procinfo.pbsd.pbi_status - 1] : "(unknown)";

	printf("bsd proc info = {\n"
	       "\tpid = %d\n"
	       "\tunique pid = %llu\n"
	       "\tppid = %d\n"
	       "\tpgid = %d\n"
	       "\tstatus = %s\n",
	    procinfo.pbsd.pbi_pid, procinfo.p_uniqidentifier.p_uniqueid, procinfo.pbsd.pbi_ppid, procinfo.pbsd.pbi_pgid,
	    status_str);

	char flagsbuf[1024] = { '\0' };
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_SYSTEM)
		strlcat(flagsbuf, "system process|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_TRACED)
		strlcat(flagsbuf, "traced|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_INEXIT)
		strlcat(flagsbuf, "exiting|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_PPWAIT)
		strlcat(flagsbuf, "pp wait|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_LP64)
		strlcat(flagsbuf, "64-bit|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_SLEADER)
		strlcat(flagsbuf, "session leader|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_CTTY)
		strlcat(flagsbuf, "has controlling tty|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_CONTROLT)
		strlcat(flagsbuf, "has controlling terminal|", 1024);
	if (procinfo.pbsd.pbi_flags & PROC_FLAG_THCWD)
		strlcat(flagsbuf, "has thread with cwd|", 1024);

	size_t len = strlen(flagsbuf);
	if (len > 0) {
		flagsbuf[len - 1] = '\0';
	} else {
		strlcat(flagsbuf, "(none)", 1024);
	}

	printf("\tflags = %s\n"
	       "\tuid = %u\n"
	       "\tsvuid = %u\n"
	       "\truid = %u\n"
	       "\tgid = %u\n"
	       "\tsvgid = %u\n"
	       "\trgid = %u\n"
	       "\tcomm name = %s\n"
	       "\tlong name = %s\n"
	       "\tcontrolling tty devnode = 0x%x\n"
	       "\tcontrolling tty pgid = %u\n"
	       "}\n",
	    flagsbuf, procinfo.pbsd.pbi_uid, procinfo.pbsd.pbi_svuid, procinfo.pbsd.pbi_ruid, procinfo.pbsd.pbi_gid,
	    procinfo.pbsd.pbi_svgid, procinfo.pbsd.pbi_rgid, procinfo.pbsd.pbi_comm, procinfo.pbsd.pbi_name,
	    procinfo.pbsd.e_tdev, procinfo.pbsd.e_tpgid);

procinfo_pressured_exit_info : {
}
	free(procargs);
	uint32_t dflags;
	retval = proc_get_dirty(pid, &dflags);
	if (retval) {
		fprintf(stderr, "proc_get_dirty(): %d: %s\n", errno, strerror(errno));
		goto procinfo_entitlements;
	}
	printf("pressured exit info = {\n"
	       "\tdirty state tracked = %d\n"
	       "\tdirty = %d\n"
	       "\tpressured-exit capable = %d\n"
	       "}\n\n",
	    !!(dflags & PROC_DIRTY_TRACKED), !!(dflags & PROC_DIRTY_IS_DIRTY),
	    !!(dflags & PROC_DIRTY_ALLOWS_IDLE_EXIT));

procinfo_entitlements : {
}
	xpc_object_t xents = get_entitlements(pid, NULL);
	if (!xents) {
		printf("entitlements = (no entitlements)\n");
		goto procinfo_cs_info;
	}
	printf("entitlements = "); // unquoted so we print it here
	launchctl_xpc_object_print(xents, NULL, 0);
	xpc_release(xents);
	printf("\n");
procinfo_cs_info : {
}
	uint32_t csflags;
	retval = csops(pid, CS_OPS_STATUS, &csflags, sizeof(uint32_t));
	if (retval) {
		printf("Could not get code signing info: %d: %s\n", errno, strerror(errno));
		goto procinfo_launchd_info;
	}

	char cs_info_str[1024];
	if (csflags & CS_VALID) {
		strlcat(cs_info_str, "valid", 1024);
	} else {
		strlcat(cs_info_str, "(none)", 1024);
	}

	if (csflags & CS_ADHOC)
		strlcat(cs_info_str, "\n\tad-hoc signed", 1024);
	if (csflags & CS_GET_TASK_ALLOW)
		strlcat(cs_info_str, "\n\tget-task-allow entitlement", 1024);
	if (csflags & CS_INSTALLER)
		strlcat(cs_info_str, "\n\tinstaller entitlement", 1024);
	if (csflags & CS_HARD)
		strlcat(cs_info_str, "\n\trefuse invalid pages", 1024);
	if (csflags & CS_KILL)
		strlcat(cs_info_str, "\n\tkill on invalid pages", 1024);
	if (csflags & CS_CHECK_EXPIRATION)
		strlcat(cs_info_str, "\n\tcheck expiration", 1024);
	if (csflags & CS_RESTRICT)
		strlcat(cs_info_str, "\n\trestrict", 1024);
	if (csflags & CS_ENFORCEMENT)
		strlcat(cs_info_str, "\n\trequire enforcement", 1024);
	if (csflags & CS_REQUIRE_LV)
		strlcat(cs_info_str, "\n\trequire library validation", 1024);
	if (csflags & CS_ALLOWED_MACHO)
		strlcat(cs_info_str, "\n\tallowed mach-o", 1024);
	if (csflags & CS_NO_UNTRUSTED_HELPERS)
		strlcat(cs_info_str, "\n\tplatform dyld", 1024);
	if (csflags & CS_ENTITLEMENTS_VALIDATED)
		strlcat(cs_info_str, "\n\tentitlements validated", 1024);
	if (csflags & CS_PLATFORM_BINARY)
		strlcat(cs_info_str, "\n\tplatform binary", 1024);

	printf("code signing info = %s\n", cs_info_str);

procinfo_launchd_info : {
}
	printf("\n");
	xpc_object_t dict, reply;
	vm_address_t addr;
	vm_size_t sz = 0x100000;
	dict = xpc_dictionary_create(NULL, NULL, 0);
	xpc_dictionary_set_int64(dict, "pid", pid);

	if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
		addr = launchctl_create_shmem(dict, sz);
	} else {
		xpc_dictionary_set_fd(dict, "fd", STDOUT_FILENO);
	}
	retval = launchctl_send_xpc_to_launchd(XPC_ROUTINE_PRINT_SERVICE, dict, &reply);
	if (retval == 0) {
		if (__builtin_available(macOS 12.0, iOS 15.0, tvOS 15.0, watchOS 8.0, *)) {
			launchctl_print_shmem(reply, addr, sz, stdout);
			vm_deallocate(mach_task_self(), addr, sz);
		}
	} else {
		if (retval == EINVAL)
			fprintf(stderr, "Bad request.\n");
		else if (retval == ENOSERVICE)
			printf("(pid %d is not managed by launchd)\n", pid);
		else
			fprintf(stderr, "Could not print service: %d: %s\n", retval, xpc_strerror(retval));
	}
	printf("\n");
	return 0;
}

int
hostinfo_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	for (int i = 0; i < HOST_MAX_SPECIAL_PORT; i++) {
		mach_port_t port;
		int retval = host_get_special_port(mach_host_self(), HOST_LOCAL_NODE, i, &port);
		if (retval || !MACH_PORT_VALID(port))
			continue;
		char *type = get_port_type(port);
		printf("host-%s port = 0x%x %s\n", mach_host_special_port_description(i), port, type);
		free(type);
	}

	exception_mask_t masks[EXC_TYPES_COUNT];
	mach_msg_type_number_t masksCnt;
	exception_handler_t old_handlers[EXC_TYPES_COUNT];
	exception_behavior_t old_behaviors[EXC_TYPES_COUNT];
	thread_state_flavor_t old_flavors[EXC_TYPES_COUNT];
	kern_return_t kr = host_get_exception_ports(mach_host_self(), 0x3ffe, masks, &masksCnt, old_handlers,
	    old_behaviors, old_flavors);
	if (kr) {
		fprintf(stderr, "host_get_exception_ports(): 0x%x\n", kr);
	} else {
		print_exception_port_info(0, masks, masksCnt, old_handlers);
	}
	return 0;
}
