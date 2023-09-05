/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <baretest/baretest.hpp>
#include <hpet.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <lapic_enabler.hpp>
#include <lapic_lvt_guard.hpp>
#include <lapic_test_tools.hpp>
#include <speculation.hpp>
#include <trace.hpp>
#include <x86asm.hpp>

#include <array>

using namespace x86;
using baretest::expectation;

static irqinfo irq_info;
static jmp_buf jump_buffer;
static auto hpet_device {hpet::get()};

constexpr uint64_t MTRR_TYPE_MASK {0xFFull};

static void irq_handle(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    longjmp(jump_buffer, 1);
}

static bool is_valid_mtrr_type(uint8_t type)
{
    // Valid types according to the Intel SDM 3, 11.11 "Memory Type Range Registers":
    // - 0: Uncacheable (UC)
    // - 1: Write Combining (WC)
    // - 4: Write-through (WT)
    // - 5: Write-protected (WP)
    // - 6: Writeback (WB)
    // All other values are reserved and using them results in #GP.
    return type == 0 or type == 1 or type == 4 or type == 5 or type == 6;
}

static bool has_uninitialized_feature_ctrl()
{
    // One example for this is the i7-3610QE found in the SINA WS/H Client IIIa.
    const auto info {x86::get_cpu_info()};
    return info == x86::cpu_info {0x6, 0x3a, 0x9};
}

void prologue()
{
    using namespace lapic_test_tools;

    mask_pic();
    write_spurious_vector(SPURIOUS_TEST_VECTOR);
    {
        irq_handler::guard handler_guard(drain_irq);
        enable_interrupts_for_single_instruction();
    }

    // We use the HPET for timeouts.
    hpet_device->enabled(true);
}

// Some machines come up with this MSR uninitialized, so the test does not really
// make sense there. We just skip it on affected CPUs for the time being.
TEST_CASE_CONDITIONAL(read_feature_control, not has_uninitialized_feature_ctrl())
{
    irq_handler::guard _(irq_handle);
    irq_info.reset();
    if (setjmp(jump_buffer) == 0) {
        wrmsr(msr::IA32_FEATURE_CONTROL, 0x5);
    }
    BARETEST_ASSERT((irq_info.vec == static_cast<uint32_t>(exception::GP)));
    BARETEST_ASSERT((irq_info.err == 0));
}

TEST_CASE(reconfigure_page_attribute_table)
{
    // The IA32_PAT MSR consists of 8 fields, PA0 - PA7, 3 bits each,
    // that define the caching.
    // Page table entries reference these types using the PCD/PWT/PAT bits.
    // For this test, it is important not to change any value
    // that is in use by our own page tables, i.e. PA0 - PA3.
    // So we simply use PA7.
    static constexpr uint64_t pat_mask = 0x700000000000000;

    auto pat = rdmsr(msr::PAT);
    BARETEST_ASSERT((pat & pat_mask) != pat_mask);

    pat |= pat_mask;
    wrmsr(msr::PAT, pat);

    BARETEST_ASSERT(rdmsr(msr::PAT) == pat);
}

TEST_CASE(rdtscp_returns_correct_tsc_aux_value_in_rcx)
{
    auto aux_val {rdmsr(IA32_TSC_AUX) + 0x42};
    wrmsr(IA32_TSC_AUX, aux_val);

    BARETEST_ASSERT(rdmsr(IA32_TSC_AUX) == aux_val);

    uint32_t aux {0};
    asm volatile("rdtscp" : "=c"(aux)::"rax", "rdx");
    BARETEST_ASSERT(aux == aux_val);
}

TEST_CASE(platform_info_is_correctly_initialized_non_zero)
{
    uint64_t platform_info {rdmsr(MSR_PLATFORM_INFO)};
    BARETEST_ASSERT(platform_info != 0);
}

TEST_CASE(mtrr_cap_valid)
{
    auto mtrr_cap {rdmsr(msr::MTRR_CAP)};
    info("MTRR_CAP: {#x}", mtrr_cap);
}

TEST_CASE(fixed_mtrrs_valid)
{
    const std::initializer_list<msr> msrs_mtrr {
        msr::MTRR_FIX_64K_00000, msr::MTRR_FIX_16K_80000, msr::MTRR_FIX_16K_A0000, msr::MTRR_FIX_4K_C0000,
        msr::MTRR_FIX_4K_C8000,  msr::MTRR_FIX_4K_D0000,  msr::MTRR_FIX_4K_D8000,  msr::MTRR_FIX_4K_E0000,
        msr::MTRR_FIX_4K_E8000,  msr::MTRR_FIX_4K_F0000,  msr::MTRR_FIX_4K_F8000,
    };

    for (const auto& msr : msrs_mtrr) {
        uint64_t val {rdmsr(msr)};
        info("MTRR: {#x} Value: {#x}", to_underlying(msr), val);
        std::array<uint8_t, sizeof(uint64_t)> msrs;
        memcpy(msrs.data(), &val, sizeof(uint64_t));
        BARETEST_ASSERT(std::all_of(msrs.begin(), msrs.end(), is_valid_mtrr_type));
    }
}

TEST_CASE(variable_range_mtrrs_valid)
{
    auto mtrr_cap {rdmsr(msr::MTRR_CAP)};

    // A variable-range MTRR consists of a base and mask MSR pair, so we have to double the range count.
    const uint32_t variable_mtrr_register_count {static_cast<uint32_t>(mtrr_cap & MTRR_CAP_VARIABLE_RANGE_COUNT_MASK)
                                                 * 2};
    const uint32_t variable_mtrr_max_offset {msr::MTRR_PHYS_BASE_0 + variable_mtrr_register_count};

    for (uint32_t idx {msr::MTRR_PHYS_BASE_0}; idx < variable_mtrr_max_offset; ++idx) {
        uint64_t val {rdmsr(idx)};

        // Variable Range MTRRs consist of base and mask register so that we have to check odd or even offsets
        // accordingly
        if (idx % 2 == 0) {
            info("MTRRPhysBase: {#x} Value: {#x}", idx, val);
            BARETEST_ASSERT(is_valid_mtrr_type(val & MTRR_TYPE_MASK));
        } else {
            info("MTRRPhysMask: {#x} Value: {#x}", idx, val);
        }
    }
}

TEST_CASE(mtrr_def_type_valid)
{
    auto mtrr_def_type {rdmsr(msr::MTRR_DEF_TYPE)};
    info("MTRR_DEF_TYPE: {#x}", mtrr_def_type);
    BARETEST_ASSERT(is_valid_mtrr_type(mtrr_def_type & MTRR_TYPE_MASK));
}

static void lapic_irq_handler(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
}

static bool has_hardware_feedback_interface()
{
    return (cpuid(CPUID_LEAF_POWER_MANAGEMENT).eax & LVL_0000_0006_EAX_HW_FEEDBACK) != 0;
}

/// Wait for an interrupt with timeout if nothing happens for too long.
static void wait_for_interrupt_for_seconds(unsigned seconds)
{
    uint64_t end_time {hpet_device->main_counter() + hpet_device->milliseconds_to_ticks(1000ull * seconds)};

    enable_interrupts();

    while (not irq_info.valid and (hpet_device->main_counter() < end_time)) {
        cpu_pause();
    }

    disable_interrupts();
}

// This test checks the Hardware Feedback Interface (HFI).
//
// We program the HFI interface and wait for an interrupt. When this interrupt arrives we expect that the CPU has
// updated the HFI structure.
//
// Intel Thread Director is a feature that builds on HFI and (for our testing purposes) has no meaningful
// differences. So we only test HFI here without enabling Thread Director.
TEST_CASE_CONDITIONAL(hfi_interrupt, has_hardware_feedback_interface())
{
    using namespace lapic_test_tools;

    irq_handler::guard handler_guard(lapic_irq_handler);
    lapic_enabler lenabler {};
    lvt_guard lvt_guard(lvt_entry::THERMAL_SENSOR, MAX_VECTOR, 0);
    irq_info.reset();

    struct hfi_global_header
    {
        uint64_t timestamp;
        uint8_t perf_cap_changed;
        uint8_t eff_cap_changed;
        uint8_t reserved[6];
    };
    static_assert(sizeof(hfi_global_header) == 16, "HFI structure layout is broken");

    alignas(PAGE_SIZE) static union
    {
        char backing_store[PAGE_SIZE];

        hfi_global_header header;
    } hfi;

    memset(hfi.backing_store, 0, sizeof(hfi.backing_store));

    // Make sure the hardware sees zero-initialized HFI buffer. It's unclear how the memory accesses are serialized, so
    // we just do mfence to make sure we have no problem.
    asm volatile("mfence" : "+m"(hfi.backing_store));

    // Set the pointer to the HFI structure.
    const uint64_t hw_feedback_ptr {IA32_HW_FEEDBACK_PTR_VALID | (uintptr_t) hfi.backing_store};
    wrmsr(msr::IA32_HW_FEEDBACK_PTR, hw_feedback_ptr);
    BARETEST_ASSERT(expectation(hw_feedback_ptr) == rdmsr(msr::IA32_HW_FEEDBACK_PTR));

    // Acknowledge any old status bits.
    wrmsr(msr::IA32_PACKAGE_THERM_STATUS, 0);

    // Ask CPU to give us interrupts for updates.
    wrmsr(msr::IA32_PACKAGE_THERM_INTERRUPT, IA32_PACKAGE_THERM_INTERRUPT_HFI_ENABLE);

    // Enable the HFI interface.
    wrmsr(msr::IA32_HW_FEEDBACK_CONFIG, IA32_HW_FEEDBACK_CONFIG_HFI_ENABLE);

    // Wait until the CPU posts status.
    wait_for_interrupt_for_seconds(1);

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT((static_cast<uint32_t>(irq_info.vec) == MAX_VECTOR));
    BARETEST_ASSERT((rdmsr(msr::IA32_PACKAGE_THERM_STATUS) & IA32_PACKAGE_THERM_STATUS_HFI_CHANGE) != 0);

    // See comment for the mfence above.
    asm volatile("mfence" : "+m"(hfi.backing_store));

    // The hardware will write a timestamp into the buffer. It should never be zero.
    info("HFI timestamp {#x}", hfi.header.timestamp);
    BARETEST_ASSERT(hfi.header.timestamp != 0);

    // Turn it off.
    wrmsr(msr::IA32_PACKAGE_THERM_STATUS, 0);
    wrmsr(msr::IA32_HW_FEEDBACK_CONFIG, 0);
    wrmsr(msr::IA32_HW_FEEDBACK_PTR, 0);

    // Wait until the hardware stops posting updates.
    while ((rdmsr(msr::IA32_PACKAGE_THERM_STATUS) & IA32_PACKAGE_THERM_STATUS_HFI_CHANGE) == 0) {
        cpu_pause();
    }
}

TEST_CASE_CONDITIONAL(spec_ctrl_msr_should_be_preserved_across_vmexits, ibrs_supported())
{
    const uint64_t spec_ctrl_host {x86::SPEC_CTRL_IBRS};

    // Turn on IBRS in the host.
    wrmsr(x86::IA32_SPEC_CTRL, spec_ctrl_host);

    // Cause a VM-Exit (cpuid exits unconditionally)
    cpuid(0);

    // Verify that IBRS is still enabled
    BARETEST_ASSERT(rdmsr(x86::IA32_SPEC_CTRL) == spec_ctrl_host);

    // Cleanup
    wrmsr(x86::IA32_SPEC_CTRL, 0);
}

BARETEST_RUN;
