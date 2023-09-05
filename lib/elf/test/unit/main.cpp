/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include "elf_example.hpp"

#include <elf.hpp>

#include <cbl/cast_helpers.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

using namespace elf;

namespace
{

const std::vector<elf64_phdr> hdrs {
    {
        .p_type = to_underlying(elf_segment_types::PT_PHDR),
        .p_flags = 0x4,
        .p_offset = 0x40,
        .p_vaddr = 0x400040ul,
        .p_paddr = 0x400040ul,
        .p_filesz = 0x1f8,
        .p_memsz = 0x1f8,
        .p_align = 0x8,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_INTERP),
        .p_flags = 0x4,
        .p_offset = 0x238,
        .p_vaddr = 0x400238ul,
        .p_paddr = 0x400238ul,
        .p_filesz = 0x1c,
        .p_memsz = 0x1c,
        .p_align = 0x1,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_LOAD),
        .p_flags = 0x5,
        .p_offset = 0x0,
        .p_vaddr = 0x400000ul,
        .p_paddr = 0x400000ul,
        .p_filesz = 0x33c8,
        .p_memsz = 0x33c8,
        .p_align = 0x200000,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_LOAD),
        .p_flags = 0x6,
        .p_offset = 0x3dd8,
        .p_vaddr = 0x603dd8ul,
        .p_paddr = 0x603dd8ul,
        .p_filesz = 0x330,
        .p_memsz = 0x460,
        .p_align = 0x200000,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_DYNAMIC),
        .p_flags = 0x6,
        .p_offset = 0x3df0,
        .p_vaddr = 0x603df0ul,
        .p_paddr = 0x603df0ul,
        .p_filesz = 0x200,
        .p_memsz = 0x200,
        .p_align = 0x8,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_NOTE),
        .p_flags = 0x4,
        .p_offset = 0x254,
        .p_vaddr = 0x400254ul,
        .p_paddr = 0x400254ul,
        .p_filesz = 0x20,
        .p_memsz = 0x20,
        .p_align = 0x4,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_GNU_EH_FRAME),
        .p_flags = 0x4,
        .p_offset = 0x2a40,
        .p_vaddr = 0x402a40ul,
        .p_paddr = 0x402a40ul,
        .p_filesz = 0x1d4,
        .p_memsz = 0x1d4,
        .p_align = 0x4,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_GNU_STACK),
        .p_flags = 0x6,
        .p_offset = 0x0,
        .p_vaddr = 0x0,
        .p_paddr = 0x0,
        .p_filesz = 0x0,
        .p_memsz = 0x0,
        .p_align = 0x10,
    },
    {
        .p_type = to_underlying(elf_segment_types::PT_GNU_RELRO),
        .p_flags = 0x4,
        .p_offset = 0x3dd8,
        .p_vaddr = 0x603dd8,
        .p_paddr = 0x603dd8,
        .p_filesz = 0x228,
        .p_memsz = 0x228,
        .p_align = 0x1,
    },
};

template <typename ELF_BIT>
bool test_is_64_bit(int ei_class)
{
    elf_header<ELF_BIT> hdr;
    hdr.e_ident.ei_class = ei_class;
    hdr.e_phnum = 0;

    elf_reader<ELF_BIT> reader {ptr_to_num(&hdr), sizeof(hdr)};
    return reader.is_64bit();
}

std::vector<uint8_t> get_elf_with_invalid_offset()
{
    std::vector<uint8_t> elf_cpy {elf_example, elf_example + elf_example_len};
    elf_reader<elf64> tmp_reader {ptr_to_num(&elf_example), elf_example_len};
    auto hdr {tmp_reader.get_elf_header()};

    auto offset_first_phdr {hdr.e_phoff};
    auto first_phdr {*tmp_reader.get_program_headers().begin()};
    first_phdr.p_offset = elf_example_len - first_phdr.p_filesz;

    memcpy(elf_cpy.data() + offset_first_phdr, &first_phdr, sizeof(first_phdr));
    return elf_cpy;
}

} // anonymous namespace

TEST(elf, elf_bitness_is_recognized_correctly)
{
    ASSERT_FALSE(test_is_64_bit<elf32>(elf_ident::ELF32));
    ASSERT_TRUE(test_is_64_bit<elf32>(elf_ident::ELF64));

    ASSERT_FALSE(test_is_64_bit<elf64>(elf_ident::ELF32));
    ASSERT_TRUE(test_is_64_bit<elf64>(elf_ident::ELF64));
}

TEST(elf, program_header_of_elf64_are_found)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};

    ASSERT_TRUE(reader.valid_segment_offsets());
    ASSERT_TRUE(reader.is_64bit());
    ASSERT_EQ(sizeof(elf64_phdr), reader.get_size_of_program_header());
    ASSERT_EQ(9ul, reader.get_number_of_program_headers());
    ASSERT_EQ(0x4010c0ul, reader.get_entry_point());
    ASSERT_EQ(64ul, reader.get_program_header_offset());
    ASSERT_TRUE(std::equal(hdrs.begin(), hdrs.end(), reader.get_program_headers().begin()));
}

TEST(elf, section_header_of_elf64_are_found)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};

    // Auto-generated from the actual file and manually verified
    std::vector<elf64_shdr> expected_sections = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0xb, 0x1, 0x2, 0x400238, 0x238, 0x1c, 0, 0, 0x1, 0},
        {0x13, 0x7, 0x2, 0x400254, 0x254, 0x20, 0, 0, 0x4, 0},
        {0x21, 0x6ffffff6, 0x2, 0x400278, 0x278, 0x30, 0x4, 0, 0x8, 0},
        {0x2b, 0xb, 0x2, 0x4002a8, 0x2a8, 0x300, 0x5, 0x1, 0x8, 0x18},
        {0x33, 0x3, 0x2, 0x4005a8, 0x5a8, 0x523, 0, 0, 0x1, 0},
        {0x3b, 0x6fffffff, 0x2, 0x400acc, 0xacc, 0x40, 0x4, 0, 0x2, 0x2},
        {0x48, 0x6ffffffe, 0x2, 0x400b10, 0xb10, 0x80, 0x5, 0x3, 0x8, 0},
        {0x57, 0x4, 0x2, 0x400b90, 0xb90, 0x48, 0x4, 0, 0x8, 0x18},
        {0x61, 0x4, 0x42, 0x400bd8, 0xbd8, 0x2a0, 0x4, 0x16, 0x8, 0x18},
        {0x6b, 0x1, 0x6, 0x400e78, 0xe78, 0x17, 0, 0, 0x4, 0},
        {0x66, 0x1, 0x6, 0x400e90, 0xe90, 0x1d0, 0, 0, 0x10, 0x10},
        {0x71, 0x1, 0x6, 0x401060, 0x1060, 0x1542, 0, 0, 0x10, 0},
        {0x77, 0x1, 0x6, 0x4025a4, 0x25a4, 0x9, 0, 0, 0x4, 0},
        {0x7d, 0x1, 0x2, 0x4025b0, 0x25b0, 0x48e, 0, 0, 0x4, 0},
        {0x85, 0x1, 0x2, 0x402a40, 0x2a40, 0x1d4, 0, 0, 0x4, 0},
        {0x93, 0x1, 0x2, 0x402c18, 0x2c18, 0x6c0, 0, 0, 0x8, 0},
        {0x9d, 0x1, 0x2, 0x4032d8, 0x32d8, 0xf0, 0, 0, 0x4, 0},
        {0xaf, 0xe, 0x3, 0x603dd8, 0x3dd8, 0x10, 0, 0, 0x8, 0x8},
        {0xbb, 0xf, 0x3, 0x603de8, 0x3de8, 0x8, 0, 0, 0x8, 0x8},
        {0xc7, 0x6, 0x3, 0x603df0, 0x3df0, 0x200, 0x5, 0, 0x8, 0x10},
        {0xd0, 0x1, 0x3, 0x603ff0, 0x3ff0, 0x10, 0, 0, 0x8, 0x8},
        {0xd5, 0x1, 0x3, 0x604000, 0x4000, 0xf8, 0, 0, 0x8, 0x8},
        {0xde, 0x1, 0x3, 0x6040f8, 0x40f8, 0x10, 0, 0, 0x8, 0},
        {0xe4, 0x8, 0x3, 0x604120, 0x4108, 0x118, 0, 0, 0x20, 0},
        {0xe9, 0x1, 0x30, 0, 0x4108, 0x8e, 0, 0, 0x1, 0x1},
        {0x1, 0x3, 0, 0, 0x4196, 0xf2, 0, 0, 0x1, 0},
    };

    ASSERT_TRUE(std::equal(expected_sections.begin(), expected_sections.end(), reader.get_section_headers().begin()));
}

TEST(elf, header_iterator_works)
{
    elf_reader<elf64>::header_iterator<elf64::elf_phdr> begin {ptr_to_num(hdrs.data())};
    elf_reader<elf64>::header_iterator<elf64::elf_phdr> end {ptr_to_num(hdrs.data()), hdrs.size()};
    ASSERT_EQ(begin, begin);
    ASSERT_NE(begin, end);
    for (auto header : hdrs) {
        auto phdr {*begin};
        ASSERT_EQ(header.p_type, phdr.p_type);
        ++begin;
    }
    ASSERT_EQ(begin, end);
}

TEST(elf, has_segment_works)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};

    ASSERT_TRUE(reader.has_dynamic_segment());
    ASSERT_FALSE(reader.has_tls_segment());
    ASSERT_TRUE(reader.has_segment<elf_segment_types::PT_LOAD>());
}

TEST(elf, offset_pointing_out_of_elf_is_rejected)
{
    const auto invalid_elf {get_elf_with_invalid_offset()};
    elf_reader<elf64> reader {ptr_to_num(invalid_elf.data()), invalid_elf.size()};
    ASSERT_FALSE(reader.valid_segment_offsets());
}

static void test_is_pt_load(uint32_t p_type, bool expect)
{
    elf32_phdr phdr32 {.p_type = p_type};
    elf64_phdr phdr64 {.p_type = p_type};
    ASSERT_EQ(expect, is_pt_load(phdr32));
    ASSERT_EQ(expect, is_pt_load(phdr64));
}

TEST(elf, is_pt_load_works)
{
    test_is_pt_load(to_underlying(elf_segment_types::PT_LOAD), true);
    test_is_pt_load(to_underlying(elf_segment_types::PT_DYNAMIC), false);
}

TEST(elf, memory_envelope_is_correct)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};
    // Lowest start address is 0x400000, highest start + memsz is 0x604238
    // Everything needs to be page-aligned, so we expect 0x205000 bytes.
    pt_load_envelope expected_envelope {0x400000, 0x205000};

    auto envelope {reader.get_pt_load_envelope()};
    EXPECT_EQ(envelope.base, expected_envelope.base);
    EXPECT_EQ(envelope.size, expected_envelope.size);
}

/**
 * Reminder: The two PT_LOAD sections are:
 *
 * {
 *      .p_offset = 0x0,
 *      .p_paddr = 0x400000ul,
 *      .p_filesz = 0x33c8,
 *      .p_memsz = 0x33c8,
 *  },
 *  {
 *      .p_offset = 0x3dd8,
 *      .p_paddr = 0x603dd8ul,
 *      .p_filesz = 0x330,
 *      .p_memsz = 0x460,
 *  }
 *
 *  We will be using those values in the unpacking tests.
 */
TEST(elf, unpacking_with_default_action_works)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};

    alignas(0x1000) std::array<char, 0x205000> fake_memory;
    static constexpr char fill_value {-1};
    fake_memory.fill(fill_value);

    auto allocate_fn = [&fake_memory](uintptr_t /*base*/, size_t /*size*/) { return fake_memory.data(); };

    auto info {reader.unpack(allocate_fn, {})};

    std::vector<elf::pt_load_envelope> expected_unused = {
        {ptr_to_num(fake_memory.data()) + 0x4000, 0x603000 - 0x404000}};
    EXPECT_EQ(info.unused_regions, expected_unused);

    // Check the real PT_LOAD binary contents
    std::vector<char> expected_segment_1(elf_example, elf_example + 0x33c8);
    std::vector<char> expected_segment_2(elf_example + 0x3dd8, elf_example + 0x3dd8 + 0x330);

    std::vector<char> actual_segment_1(fake_memory.begin(), fake_memory.begin() + 0x33c8);
    std::vector<char> actual_segment_2(fake_memory.begin() + 0x203dd8, fake_memory.begin() + 0x203dd8 + 0x330);

    EXPECT_EQ(actual_segment_1, expected_segment_1);
    EXPECT_EQ(actual_segment_2, expected_segment_2);

    // Check the zeroed out region
    EXPECT_TRUE(std::all_of(fake_memory.begin() + 0x203dd8 + 0x330, fake_memory.begin() + 0x203dd8 + 0x460,
                            [](char val) { return val == 0; }));

    // Check the rest of the memory (should be intact with the filler)
    auto is_filler = [](char val) { return val == fill_value; };
    EXPECT_TRUE(std::all_of(fake_memory.begin() + 0x33c8, fake_memory.begin() + 0x203dd8, is_filler));
    EXPECT_TRUE(std::all_of(fake_memory.begin() + 0x203dd8 + 0x460, fake_memory.end(), is_filler));
}

TEST(elf, unpacking_with_custom_callback_works)
{
    elf_reader<elf64> reader {ptr_to_num(&elf_example), elf_example_len};

    // In this test, the unpacking shouldn't touch any actual memory, so we just
    // provide a fake value without any real backing memory.
    static constexpr uintptr_t fake_base {0xc0ff33000};

    auto allocate_fn = [](uintptr_t /*base*/, size_t /*size*/) { return num_to_ptr<char>(fake_base); };

    struct callback_mock
    {
        MOCK_METHOD1(callback, bool(const pt_load_info&));
    } cb_mock;

    using testing::Return;
    EXPECT_CALL(cb_mock, callback(reader.to_pt_load_info(hdrs[2], fake_base - 0x400000)))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(cb_mock, callback(reader.to_pt_load_info(hdrs[3], fake_base - 0x400000)))
        .Times(1)
        .WillOnce(Return(true));

    auto callback_fn = std::bind(&callback_mock::callback, &cb_mock, std::placeholders::_1);
    reader.unpack(allocate_fn, callback_fn);
}
