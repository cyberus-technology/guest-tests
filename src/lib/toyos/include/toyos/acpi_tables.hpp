/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

// Use pragma pack to be compatible with MSVC

#pragma pack(push, 1)
struct acpi_rsdp
{
    std::array<char, 8> signature;
    uint8_t checksum;
    std::array<char, 6> oem_id;
    uint8_t revision;
    uint32_t rsdt;
    uint32_t length;
    uint64_t xsdt;
    uint8_t xchecksum;
    std::array<uint8_t, 3> reserved;
};
#pragma pack(pop)
static_assert(sizeof(acpi_rsdp) == 36, "RSDP size incorrect!");

#pragma pack(push, 1)
struct acpi_table_header
{
    std::array<char, 4> signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    std::array<char, 6> oem_id;
    std::array<char, 8> oem_table_id;
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};
#pragma pack(pop)
static_assert(sizeof(acpi_table_header) == 36, "ACPI table header size incorrect!");

#pragma pack(push, 1)
struct acpi_rsdt : acpi_table_header
{
    uint32_t desc_base[];

    size_t number_of_entries() const
    {
        return (length - sizeof(acpi_rsdt)) / sizeof(uint32_t);
    }
    uint32_t entry(size_t idx) const
    {
        return desc_base[idx];
    }
};
#pragma pack(pop)
static_assert(sizeof(acpi_rsdt) == 36, "RSDT size incorrect!");

#pragma pack(push, 1)
struct acpi_mcfg : acpi_table_header
{
    uint64_t reserved;

    // This following part actually repeats, but we only support one for now.

    uint64_t base;     ///< The base address of the MMCONFIG region
    uint16_t segment;  ///< The PCI segment number

    uint8_t bus_start;  ///< The first bus number that belongs to this segment.
    uint8_t bus_end;    ///< The last bus number that belongs to this segment.

    size_t busses() const
    {
        return bus_end - bus_start + 1;
    }  ///< Returns the number of busses in this segment.
};
#pragma pack(pop)
static_assert(sizeof(acpi_mcfg) == 44 + 12, "MCFG size incorrect!");

#pragma pack(push, 1)
struct acpi_gas
{
    enum address_space_t : uint8_t
    {
        SYSTEM_MEMORY = 0,
        SYSTEM_IO,
        PCI_CONFIGURATION_SPACE,
    };

    enum access_size_t : uint8_t
    {
        UNDEFINED = 0,
        BYTE,
        WORD,
        DWORD,
        QWORD,
    };

    uint8_t address_space_id;     ///< The address space where the data structure or register exists.
    uint8_t register_bit_width;   ///< The size in bits of the given register.
    uint8_t register_bit_offset;  ///< The bit offset of the given register.
    uint8_t access_size;
    uint64_t address;  ///< The 64-bit address in the given address space.
};
#pragma pack(pop)
static_assert(sizeof(acpi_gas) == 12, "GAS size incorrect!");

#pragma pack(push, 1)
// Generic Remapping Structure Type. See Intel VTD Specification 8.2.
struct acpi_remap
{
    enum type_t : uint16_t
    {
        DRHD,
        RMRR,
        ATSR,
        RHSA,
        ANDD
    };

    type_t type;
    uint16_t length;
};
#pragma pack(pop)

// Iterator for remapping entries of a DMAR table.
class dmar_remapping_iterator
{
 public:
    using iterator_category = std::input_iterator_tag;
    using value_type = const acpi_remap&;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = const acpi_remap&&;

    explicit dmar_remapping_iterator(uintptr_t cur_remap_, uintptr_t end_addr_)
        : cur_remap(cur_remap_), end_addr(end_addr_)
    {}

    const acpi_remap& operator*() const
    {
        return *reinterpret_cast<acpi_remap*>(cur_remap);
    }

    bool operator!=(const dmar_remapping_iterator&) const
    {
        return cur_remap < end_addr;
    }

    dmar_remapping_iterator& operator++()
    {
        cur_remap = cur_remap + reinterpret_cast<acpi_remap*>(cur_remap)->length;

        if (cur_remap > end_addr) {
            cur_remap = end_addr;
        }

        return *this;
    }

 private:
    uintptr_t cur_remap;
    uintptr_t end_addr;
};

#pragma pack(push, 1)
// DMA Remapping Reporting Structure. See Intel VTD Specification 8.1.
struct acpi_dmar : acpi_table_header
{
    uint8_t host_address_width;
    uint8_t flags;
    uint8_t reserved[10];

    /// Helper class to provide an iterator over dmar remapping entries.
    struct dmar_remapping_list
    {
     public:
        explicit dmar_remapping_list(const acpi_dmar& dmar_)
            : dmar(dmar_) {}

        dmar_remapping_iterator begin() const
        {
            auto dmar_addr{ reinterpret_cast<uintptr_t>(&dmar) };
            return dmar_remapping_iterator{ dmar_addr + sizeof(acpi_dmar), dmar_addr + dmar.length };
        }

        dmar_remapping_iterator end() const
        {
            auto dmar_addr{ reinterpret_cast<uintptr_t>(&dmar) };
            return dmar_remapping_iterator{ dmar_addr + dmar.length, dmar_addr + dmar.length };
        }

     private:
        const acpi_dmar& dmar;
    };

    // Returns a iterable range of remapping structures that can have different
    // types. The most notable one is the DRHD (DMA Remapping Hardware Unit Definition).
    dmar_remapping_list remapping_entries() const
    {
        return dmar_remapping_list(*this);
    }
};

// DRHD (DMA Remapping Hardware Unit Definition) Structure. See Intel VTD
// Specification 8.3.
struct acpi_drhd : acpi_remap
{
    uint8_t flags;
    uint8_t reserved;
    uint16_t segment;
    uint64_t register_base;

    bool is_catchall() const
    {
        return flags & 1;
    }

    // Dynamic device scope entries follow here
};
#pragma pack(pop)
