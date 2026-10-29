/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>

#include "elf-build-id.h"

namespace elfutil {

    namespace {

        const uint8_t ELFCLASS32_ID = 1;
        const uint8_t ELFCLASS64_ID = 2;
        const uint8_t ELFDATA2LSB_ID = 1;
        const uint32_t PT_LOAD_TYPE = 1;
        const uint32_t PT_NOTE_TYPE = 4;
        const uint32_t PF_R_FLAG = 4;
        const uint32_t NT_GNU_BUILD_ID_TYPE = 3;
        const size_t NOTE_HEADER_SZ = 12;           // namesz, descsz, type
        const uint64_t DEFAULT_PAGE_SZ = 0x1000;

        // The ELF header and program headers are within the first page of a loaded image
        const uint64_t HEADERS_WINDOW_SZ = 0x1000;
        const uint16_t PHDRS_MAX = 64;

        template<typename T>
        T read_at(const uint8_t *address) {
            T value;
            std::memcpy(&value, address, sizeof(value));
            return value;
        }

        uint64_t round_up(uint64_t value, uint64_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

        bool is_power_of_two(uint64_t value) {
            return value != 0 && (value & (value - 1)) == 0;
        }

        bool encode_hex(const uint8_t *bytes, size_t len, char *out, size_t out_size) {
            static const char *const digits = "0123456789abcdef";

            if (out_size < len * 2 + 1) {
                return false;
            }
            for (size_t i = 0; i < len; i++) {
                out[i * 2] = digits[bytes[i] >> 4];
                out[i * 2 + 1] = digits[bytes[i] & 0xf];
            }
            out[len * 2] = '\0';

            return true;
        }

        /**
         * Search a PT_NOTE segment, already known to be readable, for the GNU build id note
         */
        bool find_in_notes(const uint8_t *notes, uint64_t notes_sz, uint64_t note_align,
                           char *out, size_t out_size) {
            uint64_t offset = 0;

            // offset can pass the end when the last note's padding is not part of the segment
            while (offset <= notes_sz && notes_sz - offset >= NOTE_HEADER_SZ) {
                const uint32_t namesz = read_at<uint32_t>(notes + offset);
                const uint32_t descsz = read_at<uint32_t>(notes + offset + 4);
                const uint32_t type = read_at<uint32_t>(notes + offset + 8);

                // sums of 32-bit values and 12 cannot overflow 64 bits
                const uint64_t name_start = offset + NOTE_HEADER_SZ;
                const uint64_t desc_start = name_start + round_up(namesz, note_align);
                if (desc_start > notes_sz || descsz > notes_sz - desc_start) {
                    return false;
                }

                if (type == NT_GNU_BUILD_ID_TYPE && namesz == 4 &&
                    std::memcmp(notes + name_start, "GNU", 4) == 0) {
                    if (descsz == 0 || descsz > BUILD_ID_BYTES_MAX) {
                        return false;
                    }
                    return encode_hex(notes + desc_start, descsz, out, out_size);
                }

                offset = desc_start + round_up(descsz, note_align);
            }

            return false;
        }

        template<typename Ehdr, typename Phdr>
        bool find_build_id(const uint8_t *base, char *out, size_t out_size) {
            const Ehdr ehdr = read_at<Ehdr>(base);

            if (ehdr.e_phentsize != sizeof(Phdr) || ehdr.e_phnum == 0 ||
                ehdr.e_phnum > PHDRS_MAX) {
                return false;
            }

            const uint64_t phoff = ehdr.e_phoff;
            if (phoff > HEADERS_WINDOW_SZ ||
                phoff + static_cast<uint64_t>(ehdr.e_phnum) * sizeof(Phdr) > HEADERS_WINDOW_SZ) {
                return false;
            }

            const uint8_t *phdrs = base + phoff;

            // The lowest PT_LOAD address is where the image starts, which locates the load bias
            // (the difference between link-time and run-time addresses)
            bool has_load = false;
            uint64_t first_vaddr = 0;
            uint64_t first_align = 0;
            for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
                const Phdr phdr = read_at<Phdr>(phdrs + i * sizeof(Phdr));
                if (phdr.p_type == PT_LOAD_TYPE && (!has_load || phdr.p_vaddr < first_vaddr)) {
                    has_load = true;
                    first_vaddr = phdr.p_vaddr;
                    first_align = phdr.p_align;
                }
            }
            if (!has_load) {
                return false;
            }

            const uint64_t page = is_power_of_two(first_align) ? first_align : DEFAULT_PAGE_SZ;
            const uintptr_t bias = reinterpret_cast<uintptr_t>(base) -
                                   static_cast<uintptr_t>(first_vaddr & ~(page - 1));

            for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
                const Phdr note = read_at<Phdr>(phdrs + i * sizeof(Phdr));
                if (note.p_type != PT_NOTE_TYPE) {
                    continue;
                }

                // only read notes that lie within a readable segment
                for (uint16_t j = 0; j < ehdr.e_phnum; j++) {
                    const Phdr load = read_at<Phdr>(phdrs + j * sizeof(Phdr));
                    const bool within_load =
                            load.p_type == PT_LOAD_TYPE && (load.p_flags & PF_R_FLAG) &&
                            note.p_vaddr >= load.p_vaddr &&
                            note.p_filesz <= load.p_filesz &&
                            note.p_vaddr - load.p_vaddr <= load.p_filesz - note.p_filesz;

                    if (within_load) {
                        const uint8_t *notes = reinterpret_cast<const uint8_t *>(
                                bias + static_cast<uintptr_t>(note.p_vaddr));
                        const uint64_t note_align = (note.p_align == 8) ? 8 : 4;

                        if (find_in_notes(notes, note.p_filesz, note_align, out, out_size)) {
                            return true;
                        }
                        break;
                    }
                }
            }

            return false;
        }

    }   // namespace

    bool get_gnu_build_id(const void *base, char *out, size_t out_size) {
        if (out == nullptr || out_size == 0) {
            return false;
        }
        out[0] = '\0';

        if (base == nullptr) {
            return false;
        }

        const uint8_t *image = static_cast<const uint8_t *>(base);
        if (image[0] != 0x7f || image[1] != 'E' || image[2] != 'L' || image[3] != 'F' ||
            image[5] != ELFDATA2LSB_ID) {
            return false;
        }

        bool found = false;
        if (image[4] == ELFCLASS64_ID) {
            found = find_build_id<Ehdr64, Phdr64>(image, out, out_size);
        } else if (image[4] == ELFCLASS32_ID) {
            found = find_build_id<Ehdr32, Phdr32>(image, out, out_size);
        }

        if (!found) {
            out[0] = '\0';
        }

        return found;
    }

}   // namespace elfutil
