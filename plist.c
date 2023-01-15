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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include "xpc_private.h"

void launchctl_xpc_object_print(xpc_object_t in, const char *name, int level);

int plist_cmd(xpc_object_t *msg, int argc, char **argv, char **envp, char **apple) {
	int retval = 0;
    
    int err = 0;
    const char *wantedSegment = NULL;
    const char *wantedSection = NULL;
    const char *path = NULL;
    int fd = -1;
    size_t mappingSize = 0;
    void *mapping = NULL;
    xpc_object_t xpcObject = NULL;
    bool successfullyFound = false;
    
    if (argc < 2) {
        retval = EUSAGE;
        goto end;
    }
    
    if (argc != 3) {
        wantedSegment = SEG_TEXT;
        wantedSection = "__info_plist";
        path = argv[1];
    }
    else {
        char *specifier = argv[1];
        
        char *found = strchr(specifier, ',');
        if (found == NULL) {
            retval = EUSAGE;
            goto end;
        }
        
        *found = 0;
        
        wantedSegment = specifier;
        wantedSection = (char *)((uintptr_t)found + 1);
        path = argv[2];
    }
    
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open(): %d: %s\n", errno, strerror(errno));
        retval = 0;
        goto end;
    }
    
    struct stat status = {};
    err = fstat(fd, &status);
    if (err == -1) {
        fprintf(stderr, "fdstat(): %d: %s\n", errno, strerror(errno));
        retval = 0;
        goto end;
    }
    
    mappingSize = status.st_size;
    
    mapping = mmap(NULL, mappingSize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mapping == MAP_FAILED) {
        fprintf(stderr, "mmap(): %d: %s\n", errno, strerror(errno));
        retval = 0;
        goto end;
    }
    
    if (mappingSize < sizeof(uint32_t)) {
        fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
        retval = 0;
        goto end;
    }
    
    uint32_t magic = *(uint32_t *)mapping;
    if (magic == MH_MAGIC) {
        if (mappingSize < sizeof(struct mach_header)) {
            fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
            retval = 0;
            goto end;
        }
        
        struct mach_header *header = (struct mach_header *)mapping;
        uint32_t loadCmdCount = header->ncmds;
        uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header);
        
        struct load_command *loadCmd = (struct load_command *)loadCmdTable;
        for (uint32_t j = 0; j < loadCmdCount; j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
            if (loadCmd->cmd == LC_SEGMENT) {
                struct segment_command *segmentCmd = (struct segment_command *)loadCmd;
                if (strcmp(segmentCmd->segname, wantedSegment) == 0) {
                    uint32_t sectionCount = segmentCmd->nsects;
                    struct section *sectionTable = (struct section *)((uintptr_t)segmentCmd + sizeof(struct segment_command));
                    
                    bool superbreak = false;
                    for (uint32_t k = 0; k < sectionCount; k++) {
                        struct section *section = &(sectionTable[k]);
                        if (strcmp(section->sectname, wantedSection) == 0) {
                            size_t dataSize = (size_t)(section->size);
                            void *data = (void *)((uintptr_t)header + section->offset);
                            
                            xpcObject = xpc_create_from_plist(data, dataSize);
                            if (xpcObject != NULL) {
                                launchctl_xpc_object_print(xpcObject, NULL, 0);
                            }
                            
                            successfullyFound = true;
                            superbreak = true;
                            break;
                        }
                    }
                    if (superbreak) {
                        break;
                    }
                }
            }
        }
        
        if (!successfullyFound) {
            fprintf(stderr, "32-bit Mach-O does not have a %s,%s or is invalid.\n\n", wantedSegment, wantedSection);
            retval = 0;
            goto end;
        }
    }
    else if (magic == MH_MAGIC_64) {
        if (mappingSize < sizeof(struct mach_header_64)) {
            fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
            retval = 0;
            goto end;
        }
        
        struct mach_header_64 *header = (struct mach_header_64 *)mapping;
        uint32_t loadCmdCount = header->ncmds;
        uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header_64);
        
        struct load_command *loadCmd = (struct load_command *)loadCmdTable;
        for (uint32_t j = 0; j < loadCmdCount; j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
            if (loadCmd->cmd == LC_SEGMENT_64) {
                struct segment_command_64 *segmentCmd = (struct segment_command_64 *)loadCmd;
                if (strcmp(segmentCmd->segname, wantedSegment) == 0) {
                    uint32_t sectionCount = segmentCmd->nsects;
                    struct section_64 *sectionTable = (struct section_64 *)((uintptr_t)segmentCmd + sizeof(struct segment_command_64));
                    
                    bool superbreak = false;
                    for (uint32_t k = 0; k < sectionCount; k++) {
                        struct section_64 *section = &(sectionTable[k]);
                        if (strcmp(section->sectname, wantedSection) == 0) {
                            size_t dataSize = (size_t)(section->size);
                            void *data = (void *)((uintptr_t)header + section->offset);
                            
                            xpcObject = xpc_create_from_plist(data, dataSize);
                            if (xpcObject != NULL) {
                                launchctl_xpc_object_print(xpcObject, NULL, 0);
                            }
                            
                            successfullyFound = true;
                            superbreak = true;
                            break;
                        }
                    }
                    if (superbreak) {
                        break;
                    }
                }
            }
        }
        
        if (!successfullyFound) {
            fprintf(stderr, "64-bit Mach-O does not have a %s,%s or is invalid.\n\n", wantedSegment, wantedSection);
            retval = 0;
            goto end;
        }
    }
    else if (magic == FAT_CIGAM || magic == FAT_CIGAM_64) {
        if (mappingSize < sizeof(struct fat_header)) {
            fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
            retval = 0;
            goto end;
        }
        
        struct fat_header *fatHeader = (struct fat_header *)mapping;
        uint32_t archCount = ntohl(fatHeader->nfat_arch);
        void *archTable = (void *)((uintptr_t)fatHeader + sizeof(struct fat_header));
        
        bool supersuperbreak = false;
        for (uint32_t i = 0; i < archCount; i++) {
            void *archMapping = NULL;
            if (magic == FAT_CIGAM) {
                struct fat_arch *arch = (struct fat_arch *)((uintptr_t)archTable + (i * sizeof(struct fat_arch)));
                uint64_t archOffset = ntohl(arch->offset);
                archMapping = (void *)((uintptr_t)mapping + archOffset);
            }
            else if (magic == FAT_CIGAM_64) {
                struct fat_arch_64 *arch = (struct fat_arch_64 *)((uintptr_t)archTable + (i * sizeof(struct fat_arch_64)));
                uint64_t archOffset = ntohl(arch->offset);
                archMapping = (void *)((uintptr_t)mapping + archOffset);
            }
            
            uint32_t archMagic = *(uint32_t *)archMapping;
            if (archMagic == MH_MAGIC) {
                struct mach_header *header = (struct mach_header *)archMapping;
                uint32_t loadCmdCount = header->ncmds;
                uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header);
                
                struct load_command *loadCmd = (struct load_command *)loadCmdTable;
                for (uint32_t j = 0; j < loadCmdCount; j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
                    if (loadCmd->cmd == LC_SEGMENT) {
                        struct segment_command *segmentCmd = (struct segment_command *)loadCmd;
                        if (strcmp(segmentCmd->segname, wantedSegment) == 0) {
                            uint32_t sectionCount = segmentCmd->nsects;
                            struct section *sectionTable = (struct section *)((uintptr_t)segmentCmd + sizeof(struct segment_command));
                            
                            bool superbreak = false;
                            for (uint32_t k = 0; k < sectionCount; k++) {
                                struct section *section = &(sectionTable[k]);
                                if (strcmp(section->sectname, wantedSection) == 0) {
                                    size_t dataSize = (size_t)(section->size);
                                    void *data = (void *)((uintptr_t)header + section->offset);
                                    
                                    xpcObject = xpc_create_from_plist(data, dataSize);
                                    if (xpcObject != NULL) {
                                        launchctl_xpc_object_print(xpcObject, NULL, 0);
                                    }
                                    
                                    successfullyFound = true;
                                    superbreak = true;
                                    supersuperbreak = true;
                                    break;
                                }
                            }
                            if (superbreak) {
                                break;
                            }
                        }
                    }
                }
                
                if (!successfullyFound) {
                    fprintf(stderr, "32-bit Mach-O does not have a %s,%s or is invalid.\n\n", wantedSegment, wantedSection);
                    retval = 0;
                    goto end;
                }
            }
            else if (archMagic == MH_MAGIC_64) {
                struct mach_header_64 *header = (struct mach_header_64 *)archMapping;
                uint32_t loadCmdCount = header->ncmds;
                uintptr_t loadCmdTable = (uintptr_t)header + sizeof(struct mach_header_64);
                
                struct load_command *loadCmd = (struct load_command *)loadCmdTable;
                for (uint32_t j = 0; j < loadCmdCount; j++, loadCmd = (struct load_command *)((uintptr_t)loadCmd + loadCmd->cmdsize)) {
                    if (loadCmd->cmd == LC_SEGMENT_64) {
                        struct segment_command_64 *segmentCmd = (struct segment_command_64 *)loadCmd;
                        if (strcmp(segmentCmd->segname, wantedSegment) == 0) {
                            uint32_t sectionCount = segmentCmd->nsects;
                            struct section_64 *sectionTable = (struct section_64 *)((uintptr_t)segmentCmd + sizeof(struct segment_command_64));
                            
                            bool superbreak = false;
                            for (uint32_t k = 0; k < sectionCount; k++) {
                                struct section_64 *section = &(sectionTable[k]);
                                if (strcmp(section->sectname, wantedSection) == 0) {
                                    size_t dataSize = (size_t)(section->size);
                                    void *data = (void *)((uintptr_t)header + section->offset);
                                    
                                    xpcObject = xpc_create_from_plist(data, dataSize);
                                    if (xpcObject != NULL) {
                                        launchctl_xpc_object_print(xpcObject, NULL, 0);
                                    }
                                    
                                    successfullyFound = true;
                                    superbreak = true;
                                    supersuperbreak = true;
                                    break;
                                }
                            }
                            if (superbreak) {
                                break;
                            }
                        }
                    }
                }
                if (supersuperbreak) {
                    break;
                }
                
                if (!successfullyFound) {
                    fprintf(stderr, "64-bit Mach-O does not have a %s,%s or is invalid.\n\n", wantedSegment, wantedSection);
                    retval = 0;
                    goto end;
                }
            }
        }
        if (!successfullyFound) {
            fprintf(stderr, "Fat file does not contain valid architectures.\n");
            retval = 0;
            goto end;
        }
    }
    else {
        fprintf(stderr, "File is not a valid Mach-O or fat file.\n");
        retval = 0;
        goto end;
    }
    
end:
    if (xpcObject != NULL) {
        xpc_release(xpcObject);
    }
    if (mapping != NULL) {
        munmap(mapping, mappingSize);
    }
    if (fd != -1) {
        close(fd);
    }
    
	return retval;
}
