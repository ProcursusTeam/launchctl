#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <sysdir.h>

#include <xpc/xpc.h>

#include "xpc_private.h"

#include "launchctl.h"

int reboot3(uint64_t flags, ...);

#define RB2_USERREBOOT (0x2000000000000000llu)
#define RB2_OBLITERATE (0x4000000000000000llu)
#define RB2_FULLREBOOT (0x8000000000000000llu)
#define ITHINK_HALT    (0x8000000000000008llu)

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
