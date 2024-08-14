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
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <arpa/inet.h>
#include <errno.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <xpc/xpc.h>

#include "launchctl.h"
#include "xpc_private.h"

static xpc_object_t
get_plist_from_section_32(struct mach_header *header, const char *segmentname, const char *sectionname)
{
	xpc_object_t plist = NULL;

	uint32_t loadCmdCount = header->ncmds;
	uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header);

	struct load_command *loadCmd = (struct load_command *)loadCmdTable;
	for (uint32_t j = 0; j < loadCmdCount;
	     j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
		if (loadCmd->cmd == LC_SEGMENT) {
			struct segment_command *segmentCmd = (struct segment_command *)loadCmd;
			if (strcmp(segmentCmd->segname, segmentname) == 0) {
				uint32_t sectionCount = segmentCmd->nsects;
				struct section *sectionTable = (struct section *)((uintptr_t)segmentCmd +
				    sizeof(struct segment_command));

				for (uint32_t k = 0; k < sectionCount; k++) {
					struct section *section = &(sectionTable[k]);
					if (strcmp(section->sectname, sectionname) == 0) {
						size_t dataSize = (size_t)(section->size);
						void *secdata = (void *)((uintptr_t)header + section->offset);
						plist = xpc_create_from_plist(secdata, dataSize);
						if (plist != NULL) {
							return plist;
						}
					}
				}
			}
		}
	}

	fprintf(stderr, "32-bit Mach-O does not have a %s,%s or is invalid.\n\n", segmentname, sectionname);
	return NULL;
}

static xpc_object_t
get_plist_from_section_64(struct mach_header_64 *header, const char *segmentname, const char *sectionname)
{
	xpc_object_t plist = NULL;

	uint32_t loadCmdCount = header->ncmds;
	uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header_64);

	struct load_command *loadCmd = (struct load_command *)loadCmdTable;
	for (uint32_t j = 0; j < loadCmdCount;
	     j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
		if (loadCmd->cmd == LC_SEGMENT_64) {
			struct segment_command_64 *segmentCmd = (struct segment_command_64 *)loadCmd;
			if (strcmp(segmentCmd->segname, segmentname) == 0) {
				uint32_t sectionCount = segmentCmd->nsects;
				struct section_64 *sectionTable = (struct section_64 *)((uintptr_t)segmentCmd +
				    sizeof(struct segment_command_64));

				for (uint32_t k = 0; k < sectionCount; k++) {
					struct section_64 *section = &(sectionTable[k]);
					if (strcmp(section->sectname, sectionname) == 0) {
						size_t dataSize = (size_t)(section->size);
						void *secdata = (void *)((uintptr_t)header + section->offset);
						plist = xpc_create_from_plist(secdata, dataSize);
						if (plist != NULL) {
							return plist;
						}
					}
				}
			}
		}
	}

	fprintf(stderr, "64-bit Mach-O does not have a %s,%s or is invalid.\n\n", segmentname, sectionname);
	return NULL;
}

int
plist_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple)
{
	int err = 0;
	const char *wantedSegment = NULL;
	const char *wantedSection = NULL;
	const char *path = NULL;
	int fd = -1;
	size_t mappingSize = 0;
	void *mapping = NULL;
	xpc_object_t plist = NULL;

	if (argc < 2) {
		return EUSAGE;
	}

	if (argc != 3) {
		wantedSegment = SEG_TEXT;
		wantedSection = "__info_plist";
		path = argv[1];
	} else {
		char *specifier = argv[1];
		char *found = strchr(specifier, ',');
		if (found == NULL) {
			return EUSAGE;
		}
		*found = 0;
		wantedSegment = specifier;
		wantedSection = (char *)((uintptr_t)found + 1);
		path = argv[2];
	}

	fd = open(path, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "open(): %d: %s\n", errno, strerror(errno));
		goto end;
	}

	struct stat status = {};
	err = fstat(fd, &status);
	if (err == -1) {
		fprintf(stderr, "fdstat(): %d: %s\n", errno, strerror(errno));
		goto end;
	}

	mappingSize = status.st_size;

	mapping = mmap(NULL, mappingSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (mapping == MAP_FAILED) {
		fprintf(stderr, "mmap(): %d: %s\n", errno, strerror(errno));
		goto end;
	}

	if (mappingSize < sizeof(uint32_t) && mappingSize < sizeof(struct mach_header) &&
	    mappingSize < sizeof(struct mach_header_64) && mappingSize < sizeof(struct fat_header)) {
		fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
		goto end;
	}

	uint32_t magic = *(uint32_t *)mapping;
	if (magic == MH_MAGIC) {
		plist = get_plist_from_section_32(mapping, wantedSegment, wantedSection);
	} else if (magic == MH_MAGIC_64) {
		plist = get_plist_from_section_64(mapping, wantedSegment, wantedSection);
	} else if (magic == FAT_CIGAM || magic == FAT_CIGAM_64) {
		struct fat_header *fatHeader = (struct fat_header *)mapping;
		uint32_t archCount = ntohl(fatHeader->nfat_arch);
		void *archTable = (void *)((uintptr_t)fatHeader + sizeof(struct fat_header));

		for (uint32_t i = 0; i < archCount; i++) {
			void *archMapping = NULL;
			if (magic == FAT_CIGAM) {
				struct fat_arch *arch = (struct fat_arch *)((uintptr_t)archTable +
				    (i * sizeof(struct fat_arch)));
				uint64_t archOffset = ntohl(arch->offset);
				archMapping = (void *)((uintptr_t)mapping + archOffset);
			} else if (magic == FAT_CIGAM_64) {
				struct fat_arch_64 *arch = (struct fat_arch_64 *)((uintptr_t)archTable +
				    (i * sizeof(struct fat_arch_64)));
				uint64_t archOffset = ntohl(arch->offset);
				archMapping = (void *)((uintptr_t)mapping + archOffset);
			}

			uint32_t archMagic = *(uint32_t *)archMapping;
			if (archMagic == MH_MAGIC) {
				plist = get_plist_from_section_32(archMapping, wantedSegment, wantedSection);
				if (plist != NULL) {
					goto print;
				} else {
					goto end;
				}
			} else if (archMagic == MH_MAGIC_64) {
				plist = get_plist_from_section_64(archMapping, wantedSegment, wantedSection);
				if (plist != NULL) {
					goto print;
				} else {
					goto end;
				}
			}
		}
		fprintf(stderr, "Fat file does not contain valid architectures.\n");
		goto end;
	} else {
		fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
		goto end;
	}

print:
	launchctl_xpc_object_print(plist, NULL, 0);

end:
	if (plist != NULL) {
		xpc_release(plist);
	}
	if (mapping != NULL) {
		munmap(mapping, mappingSize);
	}
	if (fd != -1) {
		close(fd);
	}

	return 0;
}
