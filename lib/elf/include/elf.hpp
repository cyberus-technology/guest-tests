/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <math.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <type_traits>
#include <vector>

#define ELF_PANIC_UNLESS(condition, ...)                                                                               \
    do {                                                                                                               \
        if (not(condition)) {                                                                                          \
            printf("[%s:%d] Assertion failed: %s\n", __FILE__, __LINE__, #condition);                                  \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0);

namespace elf
{

struct elf32_phdr;
struct elf32_shdr;
struct elf64_phdr;
struct elf64_shdr;

/**
 * Represents the ELF64 data types.
 */
struct elf64
{
    using elf_half = uint16_t;
    using elf_word = uint32_t;
    using elf_xword = uint64_t;
    using elf_addr = uint64_t;
    using elf_off = uint64_t;

    using elf_phdr = elf64_phdr;
    using elf_shdr = elf64_shdr;
};

/**
 * Represents the ELF32 data types.
 */
struct elf32
{
    using elf_half = uint16_t;
    using elf_word = uint32_t;
    using elf_xword = uint64_t;
    using elf_addr = uint32_t;
    using elf_off = uint32_t;

    using elf_phdr = elf32_phdr;
    using elf_shdr = elf32_shdr;
};

/**
 * Describes the available ELF segment types.
 */
enum class elf_segment_types : unsigned
{
    PT_NULL = 0,
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_TLS = 7,
    PT_GNU_EH_FRAME = 0x6474e550,
    PT_GNU_STACK = 0x6474e551,
    PT_GNU_RELRO = 0x6474e552
};

enum class elf_section_types : unsigned
{
    SHT_NULL = 0,
    SHT_PROGBITS = 1,
    SHT_STRTAB = 3,
};

enum flags : uint32_t
{
    PF_X = 1ul << 0,
    PF_W = 1ul << 1,
    PF_R = 1ul << 2,
};

/**
 * At the beginning of every ELF file. Contains the magic number and some
 * additional information. Has the same format for ELF32 and ELF64.
 */
struct elf_ident
{
    enum
    {
        MAGIC = 0x464c457f,
        ELF32 = 1,
        ELF64 = 2,
    };

    uint32_t ei_magic;  ///< ELF magic number
    uint8_t ei_class;   ///< ELF32 or ELF64
    uint8_t ei_data;    ///< little or big endian
    uint8_t ei_version; ///< ELF specification version
    uint8_t ei_osabi;   ///< OS and ABI

    uint8_t ei_abiversion; ///< ABI version
    uint8_t ei_pad[7];     ///< Padding
};
static_assert(sizeof(elf_ident) == 16, "Wrong structure size for elf_ident!");

/**
 * Represents the ELF32 program header.
 */
struct elf32_phdr
{
    elf32::elf_word p_type {0};   ///< type of segment
    elf32::elf_off p_offset {0};  ///< offset from ELF file beginning
    elf32::elf_addr p_vaddr {0};  ///< virtual address of segment in memory
    elf32::elf_addr p_paddr {0};  ///< physical address of segment in memory
    elf32::elf_word p_filesz {0}; ///< bytes of segment in file
    elf32::elf_word p_memsz {0};  ///< bytes of segment in memory
    elf32::elf_word p_flags {0};  ///< segment flags
    elf32::elf_word p_align {0};  ///< alignment constraint of segment

    bool operator==(const elf32_phdr& o) const
    {
        return p_type == o.p_type and p_flags == o.p_flags and p_offset == o.p_offset and p_vaddr == o.p_vaddr
               and p_paddr == o.p_paddr and p_filesz == o.p_filesz and p_memsz == o.p_memsz and p_align == o.p_align;
    }
};
static_assert(sizeof(elf32_phdr) == 0x20, "Wrong structure size for elf32_phdr!");

/**
 * Represents the ELF64 program header.
 */
struct elf64_phdr
{
    elf64::elf_word p_type {0};    ///< type of segment
    elf64::elf_word p_flags {0};   ///< segment flags
    elf64::elf_off p_offset {0};   ///< offset from ELF file beginning
    elf64::elf_addr p_vaddr {0};   ///< virtual address of segment in memory
    elf64::elf_addr p_paddr {0};   ///< physical address of segment in memory
    elf64::elf_xword p_filesz {0}; ///< bytes of segment in file
    elf64::elf_xword p_memsz {0};  ///< bytes of segment in memory
    elf64::elf_xword p_align {0};  ///< alignment constraint of segment

    bool operator==(const elf64_phdr& o) const
    {
        return p_type == o.p_type and p_flags == o.p_flags and p_offset == o.p_offset and p_vaddr == o.p_vaddr
               and p_paddr == o.p_paddr and p_filesz == o.p_filesz and p_memsz == o.p_memsz and p_align == o.p_align;
    }
};
static_assert(sizeof(elf64_phdr) == 0x38, "Wrong structure size for elf64_phdr!");

/**
 * Represents the ELF64 section header.
 */
struct elf64_shdr
{
    uint32_t sh_name {0};              ///< offset to string in .shstrtab
    uint32_t sh_type {0};              ///< type of section header
    uint64_t sh_flags {0};             ///< section attributes
    elf64::elf_addr sh_addr {0};       ///< virtual address of the section
    elf64::elf_off sh_offset {0};      ///< offset in file
    elf64::elf_xword sh_size {0};      ///< size of the section in file
    elf64::elf_word sh_link {0};       ///< index of associated section
    elf64::elf_word sh_info {0};       ///< extra info
    elf64::elf_xword sh_addralign {0}; ///< required alignment
    elf64::elf_xword sh_entsize {0};   ///< entry size for fixed-size entries

    bool operator==(const elf64_shdr& o) const
    {
        return sh_name == o.sh_name and sh_type == o.sh_type and sh_flags == o.sh_flags and sh_addr == o.sh_addr
               and sh_offset == o.sh_offset and sh_size == o.sh_size and sh_link == o.sh_link and sh_info == o.sh_info
               and sh_addralign == o.sh_addralign and sh_entsize == o.sh_entsize;
    }
};
static_assert(sizeof(elf64_shdr) == 0x40, "Wrong structure size for elf64_shdr!");

/**
 * Checks if a given program header describes a segment of the given type.
 * \return True, if segment has type SEGMENT_TYPE, False otherwise
 */
template <typename ELF_PHDR, elf_segment_types SEGMENT_TYPE>
constexpr bool is_of_type(const ELF_PHDR& phdr)
{
    return phdr.p_type == static_cast<std::underlying_type_t<elf_segment_types>>(SEGMENT_TYPE);
}

/**
 * Checks if a given program header describes a segment of type PT_LOAD.
 * \return True, if segment has type PT_LOAD, False otherwise
 */
template <typename ELF_PHDR>
constexpr bool is_pt_load(const ELF_PHDR& phdr)
{
    return is_of_type<ELF_PHDR, elf_segment_types::PT_LOAD>(phdr);
}

/**
 * Describes the Elf header which lies at the beginning of the ELF file.
 */
template <typename ELF_TYPE>
struct elf_header
{
    using elf_half = typename ELF_TYPE::elf_half;
    using elf_word = typename ELF_TYPE::elf_word;
    using elf_addr = typename ELF_TYPE::elf_addr;
    using elf_off = typename ELF_TYPE::elf_off;

    using elf_phdr = typename ELF_TYPE::elf_phdr;
    using elf_shdr = typename ELF_TYPE::elf_shdr;

    elf_ident e_ident; ///< elf_ident structure

    elf_half e_type;      ///< object file type
    elf_half e_machine;   ///< required architecture
    elf_word e_version;   ///< ELF specification version
    elf_addr e_entry;     ///< virtual address of entry point
    elf_off e_phoff;      ///< program header table offset
    elf_off e_shoff;      ///< section header table offset
    elf_word e_flags;     ///< processor specific flags
    elf_half e_ehsize;    ///< ELF header size in bytes
    elf_half e_phentsize; ///< program header structure size
    elf_half e_phnum;     ///< number of program header entries
    elf_half e_shentsize; ///< section header structure size
    elf_half e_shnum;     ///< number of section header entries
    elf_half e_shstrndx;  ///< section header table idx of string table
};
static_assert(sizeof(elf_header<elf32>) == 0x34, "Wrong structure size for elf_header<elf32>!");
static_assert(sizeof(elf_header<elf64>) == 0x40, "Wrong structure size for elf_header<elf64>!");

/// Describes the memory needed for loadable segments
struct pt_load_envelope
{
    pt_load_envelope(uintptr_t base_, size_t size_) : base(base_), size(size_) {}

    uintptr_t base; ///< Base address of the ELF in memory
    size_t size;    ///< Amount of bytes to cover all loadable segments

    bool operator==(const pt_load_envelope& o) const { return base == o.base and size == o.size; }
};

/// Describes a loadable section (architecture-independent)
struct pt_load_info
{
    uintptr_t paddr;  ///< The start address (physical)
    uintptr_t vaddr;  ///< The start address (virtual)
    uintptr_t offset; ///< The offset of the segment in the binary
    size_t filesz;    ///< The size of the segment in the binary
    size_t memsz;     ///< The size of the segment in memory
    uint32_t flags;   ///< The flags specified for the segment

    bool operator==(const pt_load_info& o) const
    {
        return paddr == o.paddr and vaddr == o.vaddr and offset == o.offset and filesz == o.filesz and memsz == o.memsz
               and flags == o.flags;
    }
};

/// Describes the result of the unpack operation
struct unpack_info
{
    uintptr_t elf_base;    ///< Original base address of the ELF (i.e., lowest phdr.p_paddr)
    ptrdiff_t load_offset; ///< Offset of the actual base address from the original one
    uint64_t entry_point;  ///< Effective ELF entry point (honoring relocation)

    std::vector<pt_load_envelope> unused_regions; ///< List of unused regions (holes)
};

/**
 * Allocates the ELF memory envelope.
 *
 * This function is called by the unpack operation to allocate the entire
 * ELF memory envelope (covering all PT_LOAD segments).
 *
 * If relocation is supported, the return value of this function might point
 * to a memory region of the appropriate size, but moved to a window other
 * than the one specified in the ELF.
 *
 * \param addr  The base address of the memory envelope
 * \param bytes The amount of bytes to be contained in the envelope
 * \return A pointer to the beginning of the resulting memory region
 */
using alloc_fn_t = std::function<char*(uintptr_t addr, size_t bytes)>;

/**
 * Performs a user-defined action on each PT_LOAD segment.
 *
 * This function can be passed to the unpack operation if the default
 * memcpy/memset behavior is not appropriate.
 *
 * \param pt_load  The pt_load_info structure describing the segment
 *                 (relocation-aware)
 * \return True, if the segment was handled, or false, if the default action
 *         should be taken.
 */
using callback_t = std::function<bool(const pt_load_info& pt_load)>;

/**
 * Unpacks an ELF in memory.
 *
 * This function extracts all PT_LOAD segments into memory. It automatically
 * deals with 32/64-bit binaries.
 *
 * The caller needs to provide an allocation callback indicating where in memory
 * the binary should be unpacked.
 *
 * \param base     The start address of the ELF binary
 * \param size     The size of the ELF binary
 * \param allocate Allocation function for the memory envelope
 * \return An unpack_info structure reporting the relevant data about the
 *         unpacking result
 * */
inline unpack_info unpack_elf(uintptr_t base, size_t size, const alloc_fn_t& alloc);

/**
 * Unpacks an ELF in memory using custom segment handling.
 *
 * The operation is the same as in unpack_elf, except that the caller can supply
 * a callback function that is called for each PT_LOAD segment. If this callback
 * returns true, the default unpacking operation is skipped.
 *
 * \param base     The start address of the ELF binary
 * \param size     The size of the ELF binary
 * \param allocate Allocation function for the memory envelope
 * \param callback The callback to invoke on every PT_LOAD phdr
 * \return An unpack_info structure reporting the relevant data about the
 *         unpacking result
 */
inline unpack_info unpack_elf_with_callback(uintptr_t base, size_t size, const alloc_fn_t& alloc,
                                            const callback_t& callback);

/**
 * Given the address and size of an ELF file, the elf_reader can be used to
 * retrieve various information about the ELF.
 */
template <typename ELF_TYPE>
class elf_reader
{
    static constexpr auto ALIGN_ORDER {math::order_envelope(0x1000)}; // round everything to page size

public:
    using elf_phdr = typename ELF_TYPE::elf_phdr;
    using elf_shdr = typename ELF_TYPE::elf_shdr;

    elf_reader(uintptr_t addr, size_t elf_size_) : elf_addr(addr), elf_size(elf_size_)
    {
        ELF_PANIC_UNLESS(elf_size >= sizeof(hdr));
        memcpy(&hdr, reinterpret_cast<void*>(addr), sizeof(hdr));
    }

    template <typename T>
    class header_iterator
    {
    public:
        using difference_type = ssize_t;
        using value_type = T;
        using reference = T&;
        using pointer = T*;
        using iterator_category = std::forward_iterator_tag;

        header_iterator(uintptr_t hdr_addr, size_t pos = 0) : addr(hdr_addr), cur_pos(pos) {}

        bool operator==(const header_iterator& o) const { return cur_pos == o.cur_pos; }

        bool operator!=(const header_iterator& o) const { return not operator==(o); }

        T operator*() const
        {
            T ret;
            memcpy(&ret, reinterpret_cast<void*>(addr + cur_pos * sizeof(T)), sizeof(T));
            return ret;
        }

        header_iterator& operator++()
        {
            cur_pos++;
            return *this;
        }

    private:
        uintptr_t addr;
        size_t cur_pos;
    };

    template <typename T>
    class header_range
    {
    public:
        header_range(uintptr_t hdr_addr_, size_t hdr_count_) : hdr_addr(hdr_addr_), hdr_count(hdr_count_) {}

        header_iterator<T> begin() const { return {hdr_addr}; }
        header_iterator<T> end() const { return {hdr_addr, hdr_count}; }

    private:
        uintptr_t hdr_addr;
        size_t hdr_count;
    };

    /**
     * Returns an iterable range of the ELF program headers.
     */
    header_range<elf_phdr> get_program_headers() const { return {elf_addr + hdr.e_phoff, hdr.e_phnum}; }

    /*
     * Returns a vector of PT_LOAD segments.
     */
    std::vector<elf_phdr> get_pt_load_segments() const
    {
        std::vector<elf_phdr> pt_load_segments;

        const auto phdrs {get_program_headers()};
        std::copy_if(phdrs.begin(), phdrs.end(), std::back_inserter(pt_load_segments), is_pt_load<elf_phdr>);

        return pt_load_segments;
    }

    /*
     * Returns a generic pt_load_info structure from a phdr.
     *
     * The paddr is already compensated for a potential relocation.
     */
    static pt_load_info to_pt_load_info(const elf_phdr& phdr, ptrdiff_t load_offset)
    {
        return {.paddr = (phdr.p_paddr + load_offset) & math::mask(sizeof(elf_phdr::p_paddr) * 8),
                .vaddr = phdr.p_vaddr,
                .offset = phdr.p_offset,
                .filesz = phdr.p_filesz,
                .memsz = phdr.p_memsz,
                .flags = phdr.p_flags};
    }

    /*
     * Unpacks the ELF in memory.
     */
    unpack_info unpack(const alloc_fn_t& allocate, const callback_t& callback) const
    {
        const auto envelope {get_pt_load_envelope()};
        const auto load_base {allocate(envelope.base, envelope.size)};

        const ptrdiff_t load_offset {load_base - reinterpret_cast<char*>(envelope.base)};

        auto pt_load_segments {get_pt_load_segments()};

        // Unused regions between segments need to be freed. By sorting the
        // segments, it becomes a simple operation by detecting the gap between
        // the end of the last segment and the beginning of the current one.
        std::sort(pt_load_segments.begin(), pt_load_segments.end(),
                  [](const auto& phdr1, const auto& phdr2) { return phdr1.p_paddr < phdr2.p_paddr; });

        std::vector<pt_load_envelope> unused_regions;

        uintptr_t last_end {0};
        for (const auto& phdr : pt_load_segments) {
            if (const auto paddr_aligned {math::align_down(phdr.p_paddr, ALIGN_ORDER)};
                last_end and paddr_aligned > last_end) {
                // Record memory regions in between segments we did not actually populate
                unused_regions.emplace_back(last_end + load_offset, static_cast<size_t>(paddr_aligned - last_end));
            }

            const auto addr {phdr.p_paddr + load_offset};

            ELF_PANIC_UNLESS(phdr.p_memsz >= phdr.p_filesz);
            ELF_PANIC_UNLESS(std::numeric_limits<decltype(phdr.p_offset)>::max() - phdr.p_offset >= phdr.p_filesz);
            ELF_PANIC_UNLESS(phdr.p_offset + phdr.p_filesz <= elf_size);

            if (not callback or not callback(to_pt_load_info(phdr, load_offset))) {
                // Default action: Copy binary segment, zero out the rest
                memcpy(reinterpret_cast<void*>(addr), reinterpret_cast<void*>(elf_addr + phdr.p_offset), phdr.p_filesz);
                memset(reinterpret_cast<void*>(addr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
            }

            last_end = math::align_up(phdr.p_paddr + phdr.p_memsz, ALIGN_ORDER);
        }

        return {envelope.base, load_offset, get_entry_point() + load_offset, unused_regions};
    }

    /**
     * Returns the memory envelope spanning all loadable segments.
     */
    pt_load_envelope get_pt_load_envelope() const
    {
        const auto pt_load_segments {get_pt_load_segments()};

        const auto phdr_lowest_start =
            (*std::min_element(pt_load_segments.begin(), pt_load_segments.end(),
                               [](const auto& phdr1, const auto& phdr2) { return phdr1.p_paddr < phdr2.p_paddr; }));

        const auto phdr_highest_end = (*std::max_element(
            pt_load_segments.begin(), pt_load_segments.end(), [](const auto& phdr1, const auto& phdr2) {
                return phdr1.p_paddr + phdr1.p_memsz < phdr2.p_paddr + phdr2.p_memsz;
            }));

        const uintptr_t base {phdr_lowest_start.p_paddr};
        const size_t size {phdr_highest_end.p_paddr + phdr_highest_end.p_memsz - base};

        return {math::align_down(base, ALIGN_ORDER), math::align_up(size, ALIGN_ORDER)};
    }

    /**
     * Returns an iterable range of the ELF section headers.
     */
    header_range<elf_shdr> get_section_headers() const { return {elf_addr + hdr.e_shoff, hdr.e_shnum}; }

    /**
     * Returns the ELF header.
     */
    const elf_header<ELF_TYPE>& get_elf_header() const { return hdr; }

    /**
     * Returns the virtual address of the entry point.
     */
    uint64_t get_entry_point() const { return hdr.e_entry; }

    /**
     * Returns the offset to the program headers.
     */
    uint64_t get_program_header_offset() const { return hdr.e_phoff; }

    /**
     * Returns the number of program headers.
     */
    uint64_t get_number_of_program_headers() const { return hdr.e_phnum; }

    /**
     * Returns the size of a program header structure in bytes.
     */
    uint64_t get_size_of_program_header() const { return hdr.e_phentsize; }

    /**
     * Checks if the ELF is an ELF64.
     * \return True, if the ELF is an ELF64, False otherwise
     */
    bool is_64bit() const
    {
        ELF_PANIC_UNLESS(hdr.e_ident.ei_class == elf_ident::ELF64 or hdr.e_ident.ei_class == elf_ident::ELF32);
        return hdr.e_ident.ei_class == elf_ident::ELF64;
    }

    /**
     * Checks if the program header reports segment offsets pointing inside of
     * the ELF file.
     * \return True, if all offsets point inside of the ELF, False otherwise
     */
    bool valid_segment_offsets() const
    {
        for (const auto& phdr : get_program_headers()) {
            if (phdr.p_offset + phdr.p_filesz >= elf_size) {
                return false;
            }
        }

        return true;
    }

    /**
     * Check if an elf has a TLS segment in order to determine if we
     * are loading a MUSL app or our VMM.
     */
    bool has_tls_segment() const { return has_segment<elf::elf_segment_types::PT_TLS>(); }

    /**
     * Check if an elf has a dynamic segment in order to determine if a binary
     * is linked statically or not.
     */
    bool has_dynamic_segment() const { return has_segment<elf::elf_segment_types::PT_DYNAMIC>(); }

    /**
     * Checks if an elf file has a segment of type SEGMENT_TYPE
     */
    template <elf::elf_segment_types SEGMENT_TYPE>
    bool has_segment() const
    {
        auto hdr_range {get_program_headers()};

        return std::any_of(hdr_range.begin(), hdr_range.end(), is_of_type<elf64_phdr, SEGMENT_TYPE>);
    }

private:
    uintptr_t elf_addr;
    size_t elf_size {0};

    elf_header<ELF_TYPE> hdr;
};

inline unpack_info unpack_elf_with_callback(uintptr_t base, size_t size, const alloc_fn_t& alloc,
                                            const callback_t& callback)
{
    elf_reader<elf32> elf {base, size};
    return elf.is_64bit() ? elf_reader<elf64>(base, size).unpack(alloc, callback)
                          : elf_reader<elf32>(base, size).unpack(alloc, callback);
}

inline unpack_info unpack_elf(uintptr_t base, size_t size, const alloc_fn_t& alloc)
{
    return unpack_elf_with_callback(base, size, alloc, {});
}

} // namespace elf
