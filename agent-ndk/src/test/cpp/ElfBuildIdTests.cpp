/**
 * Copyright 2021-present New Relic Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "elf-build-id.h"

using namespace elfutil;

namespace {

    const uint32_t PT_LOAD_TYPE = 1;
    const uint32_t PT_NOTE_TYPE = 4;
    const uint32_t PF_X_FLAG = 1;
    const uint32_t PF_R_FLAG = 4;
    const uint32_t NT_ABI_TAG = 1;
    const uint32_t NT_BUILD_ID = 3;
    const size_t NOTE_OFFSET = 512;

    struct ImageOptions {
        uint64_t load_vaddr = 0;                    // where the file's first byte is mapped
        uint32_t load_flags = PF_R_FLAG | PF_X_FLAG;
        bool include_note_phdr = true;
        uint64_t note_vaddr_shift = 0;              // moves the note segment's vaddr
        uint64_t note_filesz_extra = 0;             // makes the note segment longer than its data
        uint64_t note_filesz_override = 0;          // sets the note segment's length when non-zero
        bool valid_magic = true;
    };

    std::vector<uint8_t> make_note(uint32_t type, const std::vector<uint8_t> &desc,
                                   const char *name = "GNU") {
        std::vector<uint8_t> note;
        const uint32_t namesz = static_cast<uint32_t>(std::strlen(name) + 1);
        const uint32_t descsz = static_cast<uint32_t>(desc.size());
        auto append = [&note](const void *data, size_t len) {
            const auto *p = static_cast<const uint8_t *>(data);
            note.insert(note.end(), p, p + len);
        };

        append(&namesz, sizeof(namesz));
        append(&descsz, sizeof(descsz));
        append(&type, sizeof(type));
        append(name, namesz);
        note.resize((note.size() + 3) & ~size_t(3));
        append(desc.data(), desc.size());
        note.resize((note.size() + 3) & ~size_t(3));

        return note;
    }

    std::vector<uint8_t> id_bytes(size_t len, uint8_t first = 0x5a) {
        std::vector<uint8_t> id(len);
        for (size_t i = 0; i < len; i++) {
            id[i] = static_cast<uint8_t>(first + i);
        }
        return id;
    }

    std::string to_hex(const std::vector<uint8_t> &bytes) {
        static const char *digits = "0123456789abcdef";
        std::string hex;
        for (uint8_t b: bytes) {
            hex += digits[b >> 4];
            hex += digits[b & 0xf];
        }
        return hex;
    }

    // Builds an image whose file offsets are identical to its memory offsets (one PT_LOAD)
    template<typename Ehdr, typename Phdr>
    std::vector<uint8_t> make_image(uint8_t elf_class, const std::vector<uint8_t> &notes,
                                    const ImageOptions &o = ImageOptions()) {
        std::vector<uint8_t> image(8192, 0);

        Ehdr ehdr = {};
        ehdr.e_ident[0] = o.valid_magic ? 0x7f : 0x00;
        ehdr.e_ident[1] = 'E';
        ehdr.e_ident[2] = 'L';
        ehdr.e_ident[3] = 'F';
        ehdr.e_ident[4] = elf_class;
        ehdr.e_ident[5] = 1;    // little endian
        ehdr.e_ident[6] = 1;    // version
        ehdr.e_ehsize = sizeof(Ehdr);
        ehdr.e_phoff = sizeof(Ehdr);
        ehdr.e_phentsize = sizeof(Phdr);
        ehdr.e_phnum = o.include_note_phdr ? 2 : 1;

        Phdr load = {};
        load.p_type = PT_LOAD_TYPE;
        load.p_flags = o.load_flags;
        load.p_offset = 0;
        load.p_vaddr = o.load_vaddr;
        load.p_filesz = image.size();
        load.p_memsz = image.size();
        load.p_align = 0x1000;

        Phdr note = {};
        note.p_type = PT_NOTE_TYPE;
        note.p_flags = PF_R_FLAG;
        note.p_offset = NOTE_OFFSET;
        note.p_vaddr = o.load_vaddr + NOTE_OFFSET + o.note_vaddr_shift;
        note.p_filesz = o.note_filesz_override != 0 ? o.note_filesz_override
                                                    : notes.size() + o.note_filesz_extra;
        note.p_memsz = note.p_filesz;
        note.p_align = 4;

        std::memcpy(&image[0], &ehdr, sizeof(ehdr));
        std::memcpy(&image[sizeof(Ehdr)], &load, sizeof(load));
        if (o.include_note_phdr) {
            std::memcpy(&image[sizeof(Ehdr) + sizeof(Phdr)], &note, sizeof(note));
        }
        std::memcpy(&image[NOTE_OFFSET], notes.data(), notes.size());

        return image;
    }

    std::vector<uint8_t> make_image64(const std::vector<uint8_t> &notes,
                                      const ImageOptions &o = ImageOptions()) {
        return make_image<Ehdr64, Phdr64>(2, notes, o);
    }

    std::vector<uint8_t> make_image32(const std::vector<uint8_t> &notes,
                                      const ImageOptions &o = ImageOptions()) {
        return make_image<Ehdr32, Phdr32>(1, notes, o);
    }

    std::string read_build_id(const std::vector<uint8_t> &image, bool *found = nullptr) {
        char out[BUILD_ID_HEX_SZ_MAX];
        std::memset(out, 'x', sizeof(out));
        bool ok = get_gnu_build_id(image.data(), out, sizeof(out));
        if (found != nullptr) {
            *found = ok;
        }
        EXPECT_NE(nullptr, std::memchr(out, '\0', sizeof(out))) << "out must be NUL terminated";
        if (!ok) {
            EXPECT_STREQ("", out) << "out must be empty when no id is found";
        }
        return std::string(out);
    }

}   // namespace

TEST(ElfBuildIdTest, Reads64BitBuildId) {
    auto id = id_bytes(20);
    bool found = false;

    EXPECT_EQ(to_hex(id), read_build_id(make_image64(make_note(NT_BUILD_ID, id)), &found));
    EXPECT_TRUE(found);
}

TEST(ElfBuildIdTest, Reads32BitBuildId) {
    auto id = id_bytes(20, 0x10);
    bool found = false;

    EXPECT_EQ(to_hex(id), read_build_id(make_image32(make_note(NT_BUILD_ID, id)), &found));
    EXPECT_TRUE(found);
}

TEST(ElfBuildIdTest, SkipsOtherNotesBeforeBuildId) {
    auto id = id_bytes(16);
    auto notes = make_note(NT_ABI_TAG, id_bytes(16, 0x01));
    auto build_id_note = make_note(NT_BUILD_ID, id);
    notes.insert(notes.end(), build_id_note.begin(), build_id_note.end());

    EXPECT_EQ(to_hex(id), read_build_id(make_image64(notes)));
}

TEST(ElfBuildIdTest, AppliesLoadBias) {
    // image mapped at a non-zero link address; the note's vaddr is relative to it
    ImageOptions options;
    options.load_vaddr = 0x10000;
    auto id = id_bytes(20, 0x40);

    EXPECT_EQ(to_hex(id), read_build_id(make_image64(make_note(NT_BUILD_ID, id), options)));
    EXPECT_EQ(to_hex(id), read_build_id(make_image32(make_note(NT_BUILD_ID, id), options)));
}

TEST(ElfBuildIdTest, ReadsLargestSupportedId) {
    auto id = id_bytes(BUILD_ID_BYTES_MAX);

    EXPECT_EQ(to_hex(id), read_build_id(make_image64(make_note(NT_BUILD_ID, id))));
}

TEST(ElfBuildIdTest, NoBuildIdWithoutNotePhdr) {
    ImageOptions options;
    options.include_note_phdr = false;
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20)), options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, NoBuildIdWhenOnlyOtherNotes) {
    bool found = true;

    read_build_id(make_image64(make_note(NT_ABI_TAG, id_bytes(16))), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, IgnoresBuildIdNoteWithOtherOwner) {
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20), "ABC")), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsEmptyBuildId) {
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, {})), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsOversizedBuildId) {
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(BUILD_ID_BYTES_MAX + 1))), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsNoteOutsideLoadSegment) {
    ImageOptions options;
    options.note_vaddr_shift = 0x100000;
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20)), options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsNoteSegmentLongerThanLoadSegment) {
    ImageOptions options;
    options.note_filesz_extra = 0x10000;
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20)), options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsNoteInUnreadableSegment) {
    ImageOptions options;
    options.load_flags = PF_X_FLAG;
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20)), options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsNoteClaimingHugeDescriptor) {
    // descsz close to UINT32_MAX must not overflow the bounds checks
    auto note = make_note(NT_BUILD_ID, id_bytes(20));
    const uint32_t huge = 0xfffffff0;
    std::memcpy(&note[4], &huge, sizeof(huge));
    bool found = true;

    read_build_id(make_image64(note), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, DoesNotReadPastNoteSegmentWhenPaddingIsMissing) {
    // The segment ends inside the first note's trailing padding, so the walk must stop there
    // even though a valid build id note follows in memory
    auto notes = make_note(NT_ABI_TAG, id_bytes(5));
    auto build_id_note = make_note(NT_BUILD_ID, id_bytes(20));
    notes.insert(notes.end(), build_id_note.begin(), build_id_note.end());

    ImageOptions options;
    options.note_filesz_override = 12 + 4 + 5;  // header + "GNU\0" + unpadded descriptor
    bool found = true;

    read_build_id(make_image64(notes, options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsBadMagic) {
    ImageOptions options;
    options.valid_magic = false;
    bool found = true;

    read_build_id(make_image64(make_note(NT_BUILD_ID, id_bytes(20)), options), &found);
    EXPECT_FALSE(found);
}

TEST(ElfBuildIdTest, RejectsTooSmallOutputBuffer) {
    auto image = make_image64(make_note(NT_BUILD_ID, id_bytes(20)));
    char out[10];
    std::memset(out, 'x', sizeof(out));

    EXPECT_FALSE(get_gnu_build_id(image.data(), out, sizeof(out)));
    EXPECT_STREQ("", out);
}

TEST(ElfBuildIdTest, HandlesNullArguments) {
    auto image = make_image64(make_note(NT_BUILD_ID, id_bytes(20)));
    char out[BUILD_ID_HEX_SZ_MAX];

    EXPECT_FALSE(get_gnu_build_id(nullptr, out, sizeof(out)));
    EXPECT_STREQ("", out);
    EXPECT_FALSE(get_gnu_build_id(image.data(), nullptr, 0));
    EXPECT_FALSE(get_gnu_build_id(image.data(), out, 0));
}
