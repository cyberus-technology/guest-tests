/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/interval_map.hpp>
#include <compiler.h>

#include <algorithm>
#include <iterator>

#include <acpi/tables.hpp>

#include <arch.hpp>
#include <cbl/interval.hpp>
#include <math.hpp>
#include <span.hpp>

namespace hedron
{

class hip
{
protected:
    static constexpr uint32_t SIGNATURE_VALID = 0x4e524448;

    static constexpr uint32_t FEATURE_VMX = 1u << 1;
    static constexpr uint32_t FEATURE_SVM = 1u << 2;
    static constexpr uint32_t FEATURE_UEFI = 1u << 3;

public:
    static constexpr uint32_t API_VERSION = 13000;

    struct __PACKED__ data
    {
        uint32_t signature_ = SIGNATURE_VALID;
        uint16_t checksum_ = 0;
        uint16_t length_ = sizeof(*this);

        struct __PACKED__
        {
            uint16_t offset_;
            uint16_t size_;
        } cpu_;

        struct __PACKED__
        {
            uint16_t offset_;
            uint16_t size_;
        } mem_;

        uint32_t feature_flags_;
        uint32_t api_version_;

        uint32_t sel_;
        uint32_t exc_;
        uint32_t vmi_;
        uint32_t page_sizes_;
        uint32_t utcb_sizes_;
        uint32_t tsc_freq_;
        uint64_t mcfg_base_;
        uint64_t mcfg_size_;
        uint64_t dmar_table_;

        uint64_t cap_vmx_sec_exec_;

        uint64_t xsdt_rsdt_table_;

        // See ACPI Specification, Section 5.2.3.1 "Generic Address Structure (GAS)"
        struct __PACKED__
        {
            uint8_t address_space_id;
            uint8_t bit_width;
            uint8_t bit_offset;
            uint8_t access_size;
            uint64_t address;
        } pm1a_cnt, pm1b_cnt;
    };

    // Avoid issues with padding and make sure we get exactly what Hedron provides.
    static_assert(sizeof(data) == 112, "HIP data layout");

    struct __PACKED__ cpu_descriptor
    {
        uint8_t flags;
        uint8_t thread;
        uint8_t core;
        uint8_t package;
        uint8_t acpi_id;
        uint8_t apic_id;
        uint8_t reserved[2];

        bool enabled() const { return flags & 0x1; }
    };

    struct __PACKED__ mem_descriptor
    {
        uint64_t address;
        uint64_t size;

        enum class mem_type : int32_t
        {
            AVAILABLE = 1,
            RESERVED = 2,
            ACPI_RECLAIM = 3,
            ACPI_NVS = 4,
            MICROHYPERVISOR = -1,
            MULTIBOOT_MOD = -2,
            RUNTIME_HEAP = -3,
            UNASSIGNED = -4,
        } type;

        uint32_t aux;

        cbl::interval as_interval() const { return cbl::interval::from_size(address, size); }
    };

protected:
    const cpu_descriptor* first_cpu() const;
    const mem_descriptor* first_mem() const;

    size_t num_mem_descriptors() const { return (hip_.length_ - hip_.mem_.offset_) / hip_.mem_.size_; }

    const data& hip_;

    span<const cpu_descriptor> cpu_descriptors_;
    span<const mem_descriptor> mem_descriptors_;

    template <typename hip_gas_t>
    acpi_gas to_gas(const hip_gas_t& gas) const
    {
        return {.address_space_id = gas.address_space_id,
                .register_bit_width = gas.bit_width,
                .register_bit_offset = gas.bit_offset,
                .access_size = gas.access_size,
                .address = gas.address};
    }

public:
    hip() = delete;

    explicit hip(const data& h)
        : hip_ {h}
        , cpu_descriptors_ {first_cpu(), num_cpu_descriptors()}
        , mem_descriptors_ {first_mem(), num_mem_descriptors()}
    {}

    bool valid() const;

    /**
     * Check if the version number matches our expectations.
     *
     * The version number field is a major/minor identifier:
     *
     * - The thousands portion indicates breaking changes
     * - The rest indicates feature changes maintaining backward compatibility
     *
     * This means that we always expect the thousands indicator to be an exact
     * match, and the remainder to be greater or equal to what we expect.
     *
     * For example:
     *   HIP: 2000, Supernova: 2000 -> Good
     *   HIP: 2002, SuperNOVA: 2001 -> Good
     *   HIP: 2000, SuperNOVA: 1000 -> Bad
     *   HIP: 1000, SuperNOVA: 2000 -> Bad
     */
    bool version_ok() const;

    uint32_t api_version() const { return hip_.api_version_; }

    bool has_vmx() const { return !!(hip_.feature_flags_ & FEATURE_VMX); }
    bool has_svm() const { return !!(hip_.feature_flags_ & FEATURE_SVM); }
    bool has_uefi() const { return !!(hip_.feature_flags_ & FEATURE_UEFI); }

    unsigned num_cpu_descriptors() const { return (hip_.mem_.offset_ - hip_.cpu_.offset_) / hip_.cpu_.size_; }

    unsigned cpus() const;

    unsigned root_pd() const { return hip_.exc_ + 0; }
    unsigned root_ec() const { return hip_.exc_ + 1; }
    unsigned root_sc() const { return hip_.exc_ + 2; }

    unsigned first_free_selector() const { return hip_.exc_ + 3 + cpus(); }
    unsigned first_gsi() const { return num_cpu_descriptors(); }

    unsigned max_cap_sel() const { return hip_.sel_; }

    uint64_t tsc_freq_khz() const { return hip_.tsc_freq_; }

    uint64_t mcfg_base() const { return hip_.mcfg_base_; }
    uint64_t mcfg_size() const { return hip_.mcfg_size_; }
    cbl::interval mcfg_ival() const { return cbl::interval::from_size(mcfg_base(), mcfg_size()); }

    /**
     * \brief Return the physical address of the DMAR ACPI table.
     *
     * This function is DEPRECATED and will be removed. Please use xsdt_rsdt_table() to find other ACPI tables.
     */
    uint64_t dmar_table() const { return hip_.dmar_table_; }

    /**
     * \brief Return the physical address of the XSDT or RSDT ACPI system descriptor table.
     *
     * If a XSDT is available in the system, its address will be returned. Otherwise, we fall back to the RSDT. To
     * distinguish what kind of table is returned, the caller needs to check the ACPI table signature.
     */
    uint64_t xsdt_rsdt_table() const { return hip_.xsdt_rsdt_table_; }

    acpi_gas pm1a_cnt() const { return to_gas(hip_.pm1a_cnt); }
    acpi_gas pm1b_cnt() const { return to_gas(hip_.pm1b_cnt); }

    /**
     * \brief Get all secondary execution controls which can be enabled.
     *
     * The 64 bit field is split as follows:
     *   bit  0-31 determine which secondary execution control fields are allowed to be '0'
     *   bit 32-63 determine which fields are allowed to be '1' (turned on)
     *
     * \return Secondary execution controls which can be turned on.
     */
    uint32_t allowed_secondary_exec_ctrls() const { return hip_.cap_vmx_sec_exec_ >> 32; }

    const span<const mem_descriptor>& mem_descriptors() const { return mem_descriptors_; }
    span<const mem_descriptor>& mem_descriptors() { return mem_descriptors_; }

    const span<const cpu_descriptor>& cpu_descriptors() const { return cpu_descriptors_; }
    span<const cpu_descriptor>& cpu_descriptors() { return cpu_descriptors_; }

    auto merged_mem_descriptors() const
    {
        using mem_type = mem_descriptor::mem_type;
        cbl::interval_vector<mem_type> mem_map(mem_type::RESERVED);

        for (const auto& desc : mem_descriptors()) {
            if (desc.type == mem_type::AVAILABLE) {
                mem_map.insert(cbl::interval::from_size(desc.address, desc.size), mem_type::AVAILABLE);
            }
        }

        for (const auto& desc : mem_descriptors()) {
            if (desc.type != mem_type::AVAILABLE) {
                mem_map.insert(cbl::interval::from_size(desc.address, desc.size), desc.type);
            }
        }

        return mem_map;
    }

    const std::vector<mem_descriptor> get_multiboot_modules() const
    {
        std::vector<mem_descriptor> modules;

        std::copy_if(mem_descriptors().begin(), mem_descriptors().end(), std::back_inserter(modules),
                     [](const mem_descriptor& e) { return e.type == mem_descriptor::mem_type::MULTIBOOT_MOD; });

        return modules;
    }
};

} // namespace hedron
