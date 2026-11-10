/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _AGENT_NDK_ELF_BUILD_ID_H
#define _AGENT_NDK_ELF_BUILD_ID_H

#include <cstddef>
#include <cstdint>

/**
 * Minimal ELF definitions, so the build id can be read from an image mapped in memory without
 * depending on <elf.h> (not available on every host the tests run on).
 */
namespace elfutil {

    // Longest GNU build id supported, in bytes (SHA1 is 20, UUID/MD5 is 16)
    static const size_t BUILD_ID_BYTES_MAX = 64;

    // Size of the buffer needed to hold the hex encoding of any supported build id, with its NUL
    static const size_t BUILD_ID_HEX_SZ_MAX = BUILD_ID_BYTES_MAX * 2 + 1;

    struct Ehdr32 {
        uint8_t e_ident[16];
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint32_t e_entry;
        uint32_t e_phoff;
        uint32_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    };

    struct Phdr32 {
        uint32_t p_type;
        uint32_t p_offset;
        uint32_t p_vaddr;
        uint32_t p_paddr;
        uint32_t p_filesz;
        uint32_t p_memsz;
        uint32_t p_flags;
        uint32_t p_align;
    };

    struct Ehdr64 {
        uint8_t e_ident[16];
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_version;
        uint64_t e_entry;
        uint64_t e_phoff;
        uint64_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
    };

    struct Phdr64 {
        uint32_t p_type;
        uint32_t p_flags;
        uint64_t p_offset;
        uint64_t p_vaddr;
        uint64_t p_paddr;
        uint64_t p_filesz;
        uint64_t p_memsz;
        uint64_t p_align;
    };

    /**
     * Read the GNU build id (the .note.gnu.build-id note) of the ELF image mapped at base.
     *
     * Uses only the program headers, which are mapped with the image, and never reads outside
     * of the image's readable PT_LOAD segments. Does not allocate or lock, so it is usable
     * from a signal handler.
     *
     * @param base Address the image is mapped at (dladdr's dli_fbase)
     * @param out Receives the lower-case hex encoded id, NUL terminated. Empty on failure.
     * @param out_size Size of out; BUILD_ID_HEX_SZ_MAX holds any supported id
     * @return true if the image has a build id and it was written to out
     */
    bool get_gnu_build_id(const void *base, char *out, size_t out_size);

}   // namespace elfutil

#endif // _AGENT_NDK_ELF_BUILD_ID_H
