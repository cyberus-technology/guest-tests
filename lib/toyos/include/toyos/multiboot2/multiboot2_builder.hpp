/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "multiboot2.hpp"

namespace multiboot2
{

/**
 * Simple MBI2 structure builder
 *
 * The idea is that code can use this to add specific information to the structure
 * without knowing exactly how it needs to be arranged in the end.
 *
 * For example, the boot command line can be added using add_boot_cmdline("foobar").
 *
 * When all elements are added, the generate() function creates a functional
 * structure with correct header and overall layout and returns it as a byte
 * vector.
 */
class mbi2_builder
{
public:
    /// Add the string given by cmdline as boot cmdline.
    void add_boot_cmdline(const std::string& cmdline)
    {
        std::vector<char> raw(cmdline.begin(), cmdline.end());
        raw.push_back(0);
        add_tag(mbi2_cmdline(), raw);
    }

    /// Add the ACPI 2.0 RSDP table pointed to by rsdp_ptr as the respective tag.
    void add_rsdp(void* rsdp_ptr)
    {
        constexpr size_t acpi_20_rsdp_size {36};
        std::array<uint8_t, acpi_20_rsdp_size> rsdp;
        memcpy(rsdp.data(), rsdp_ptr, acpi_20_rsdp_size);

        add_tag(mbi2_rsdp2(), rsdp);
    }

    /// Add a list of memory regions.
    void add_memory(const std::vector<mmap_entry>& entries)
    {
        std::vector<uint8_t> entries_raw;
        entries_raw.reserve(entries.size() * sizeof(mmap_entry));

        for (const auto& e : entries) {
            insert_bytewise(e, entries_raw);
        }

        add_tag(mbi2_mmap(), entries_raw);
    }

    /// Add a boot module.
    void add_boot_module(uint32_t start, uint32_t end, const std::string& cmdline)
    {
        std::vector<char> raw(cmdline.begin(), cmdline.end());
        raw.push_back(0);
        add_tag(mbi2_boot_module(start, end), raw);
    }

    /// Add the 64-bit EFI system table pointer.
    void add_system_table(void* system_table) { add_tag(mbi2_efi_system_table(uint64_t(system_table))); }

    void add_image_load_base(uint32_t load_base) { add_tag(mbi2_image_load_base(load_base)); }

    /// Return the final MBI2 structure as byte vector.
    std::vector<uint8_t> build()
    {
        std::vector<uint8_t> ret;

        auto tags_size {std::accumulate(raw_data.begin(), raw_data.end(), 0,
                                        [](int sum, const auto& item) { return sum + item.size(); })};

        mbi2_fixed fix;
        fix.total_size = sizeof(fix) + tags_size + sizeof(mbi2_tag);

        ret.reserve(fix.total_size);

        insert_bytewise(fix, ret);

        for (const auto& item_raw : raw_data) {
            std::copy(item_raw.begin(), item_raw.end(), std::back_inserter(ret));
        }

        insert_bytewise(mbi2_nulltag {}, ret);

        return ret;
    }

private:
    template <typename T>
    void insert_bytewise(const T& item, std::vector<uint8_t>& vec)
    {
        std::array<uint8_t, sizeof(T)> raw;
        memcpy(raw.data(), &item, sizeof(T));
        std::copy(raw.begin(), raw.end(), std::back_inserter(vec));
    }

    template <typename TAG, typename PAYLOAD>
    void add_tag(const TAG& tag, const PAYLOAD& payload)
    {
        auto size {sizeof(TAG) + payload.size()};
        size_t aligned_size {math::align_up(size, math::order_max(mbi2_tag::ALIGNMENT))};

        std::vector<uint8_t> tag_raw;
        tag_raw.reserve(aligned_size);

        // Make a copy so we can fill the size before storing the tag
        auto tag_copy {tag};
        tag_copy.size = size;

        insert_bytewise(tag_copy, tag_raw);

        std::vector<uint8_t> payload_raw(std::begin(payload), std::end(payload));
        std::copy(payload_raw.begin(), payload_raw.end(), std::back_inserter(tag_raw));

        // Resize to create the zero padding for 8-byte alignment
        tag_raw.resize(aligned_size, 0);

        raw_data.push_back(tag_raw);
    }

    template <typename TAG>
    void add_tag(const TAG& tag)
    {
        add_tag(tag, std::vector<unsigned char>());
    }

    std::vector<std::vector<uint8_t>> raw_data;
};

} // namespace multiboot2
