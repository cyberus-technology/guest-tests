/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <assert.h>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/int_guard.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/util/cast_helpers.hpp>
#include <toyos/x86/x86asm.hpp>

using x86::msr;

void lapic_test_tools::mask_pic()
{
    outb(PIC0_DATA, 0xff);
    outb(PIC1_DATA, 0xff);
}

void lapic_test_tools::write_to_register(uintptr_t address, uint32_t value)
{
    *num_to_ptr<volatile uint32_t>(address) = value;
}

uint32_t lapic_test_tools::read_from_register(uintptr_t address)
{
    return *num_to_ptr<volatile uint32_t>(address);
}

void lapic_test_tools::drain_irq(intr_regs* regs)
{
    info("draining interrupt: {}", regs->vector);
    send_eoi();
}

bool lapic_test_tools::lapic_send_pending()
{
    return *num_to_ptr<volatile uint32_t>(LAPIC_ICR_LOW) & LAPIC_DLV_STS_MASK;
}

void lapic_test_tools::wait_until_ready_for_ipi()
{
    while (lapic_send_pending()) {
    }
}

void lapic_test_tools::write_reg_generic(uintptr_t address, uint32_t mask, uint32_t shift, uint32_t value)
{
    uint32_t reg_value = *num_to_ptr<volatile uint32_t>(address);
    reg_value = reg_value & ~(mask << shift);
    *num_to_ptr<volatile uint32_t>(address) = reg_value | ((value & mask) << shift);
}

void lapic_test_tools::write_spurious_vector(uint8_t value)
{
    write_reg_generic(LAPIC_SVR, SVR_VECTOR_MASK, 0, value);
}

void lapic_test_tools::lapic_set_task_priority(uint8_t priority, bool use_mmio)
{
    if (use_mmio) {
        write_reg_generic(LAPIC_TPR, LAPIC_TPR_CLASS_MASK, LAPIC_TPR_CLASS_SHIFT, priority);
    }
    else {
        set_cr8(priority);
    }
}

void lapic_test_tools::write_lvt_generic(lvt_entry entry, uint32_t mask, uint32_t shift, uint32_t value)
{
    uint32_t lvt_entry_address = 0xfee00000 | static_cast<uint32_t>(entry);
    write_reg_generic(lvt_entry_address, mask, shift, value);
}

void lapic_test_tools::write_lvt_vector(lvt_entry entry, uint32_t vector)
{
    write_lvt_generic(entry, LVT_VECTOR_MASK, 0, vector);
}

void lapic_test_tools::write_lvt_mask(lvt_entry entry, uint32_t mask)
{
    write_lvt_generic(entry, LVT_MASK_MASK, LVT_MASK_SHIFT, mask);
}

void lapic_test_tools::write_lvt_mode(lvt_entry entry, uint32_t mode)
{
    write_lvt_generic(entry, LVT_MODE_MASK, LVT_MODE_SHIFT, mode);
}

void lapic_test_tools::write_lvt_entry(lvt_entry entry, lvt_entry_t data)
{
    uintptr_t lvt_address = 0xfee00000 | static_cast<uint32_t>(entry);
    uint32_t lvt_data = read_from_register(lvt_address);
    // put vector into data
    lvt_data &= ~LVT_VECTOR_MASK;
    lvt_data |= data.vector;
    // put mode into data
    lvt_data &= ~(LVT_MODE_MASK << LVT_MODE_SHIFT);
    lvt_data |= ((data.mode & LVT_MODE_MASK) << LVT_MODE_SHIFT);
    // put mask into data
    lvt_data &= ~(LVT_MASK_MASK << LVT_MASK_SHIFT);
    lvt_data |= ((data.mask & LVT_MASK_MASK) << LVT_MASK_SHIFT);
    // write to register
    write_to_register(lvt_address, lvt_data);
}

void lapic_test_tools::write_divide_conf(uint32_t conf)
{
    uint32_t config;
    switch (conf) {
        case 2:
            config = 0b0000;
            break;
        case 4:
            config = 0b0001;
            break;
        case 8:
            config = 0b0010;
            break;
        case 16:
            config = 0b0011;
            break;
        case 32:
            config = 0b1000;
            break;
        case 64:
            config = 0b1001;
            break;
        case 128:
            config = 0b1010;
            break;
        default:
            config = 0b1011;
    }
    *num_to_ptr<volatile uint32_t>(LAPIC_DIVIDE_CONF) = config;
}

void lapic_test_tools::stop_lapic_timer()
{
    write_to_register(LAPIC_INIT_COUNT, 0);
}

void lapic_test_tools::send_eoi()
{
    write_to_register(LAPIC_EOI, 0);
}

static bool rtc_update_in_progress()
{
    constexpr uint32_t CMOS_CONF{ 0x70 };
    constexpr uint32_t CMOS_DATA{ 0x71 };
    constexpr uint8_t RTC_STATUS_REG_A{ 0x0A };
    constexpr uint8_t UPDATE_IN_PROGRESS_BIT{ 0x7 };

    outb(CMOS_CONF, RTC_STATUS_REG_A);
    return inb(CMOS_DATA) >> UPDATE_IN_PROGRESS_BIT;
}

static void wait_till_next_second()
{
    while (not rtc_update_in_progress()) {
    }
    while (rtc_update_in_progress()) {
    }
}

uint32_t lapic_test_tools::ticks_per_second(uint32_t divisor)
{
    static uint32_t ticks_per_second = 0;
    if (not ticks_per_second) {
        int_guard _;

        uint32_t previous_divisor{ read_from_register(LAPIC_DIVIDE_CONF) };
        uint32_t previous_lvt_timer{ read_from_register(LAPIC_LVT_TIMER) };
        uint32_t previous_init_count{ read_from_register(LAPIC_INIT_COUNT) };

        write_divide_conf(1);
        write_lvt_entry(lvt_entry::TIMER, { .vector = 0x0, .mode = lvt_modes::ONESHOT, .mask = 0x1 });

        wait_till_next_second();
        write_to_register(LAPIC_INIT_COUNT, LAPIC_MAX_COUNT);
        write_to_register(LAPIC_CURR_COUNT, LAPIC_MAX_COUNT);
        wait_till_next_second();
        ticks_per_second = LAPIC_MAX_COUNT - *num_to_ptr<volatile uint32_t>(LAPIC_CURR_COUNT);

        write_to_register(LAPIC_DIVIDE_CONF, previous_divisor);
        write_to_register(LAPIC_LVT_TIMER, previous_lvt_timer);
        write_to_register(LAPIC_INIT_COUNT, previous_init_count);
    }
    return ticks_per_second * divisor;
}

void lapic_test_tools::enable_periodic_timer(uint32_t interrupt_vector, uint32_t period_in_ms)
{
    constexpr uint32_t LAPIC_DIVISOR{ 1 };

    disable_interrupts();

    write_divide_conf(LAPIC_DIVISOR);
    write_lvt_entry(lvt_entry::TIMER, { .vector = interrupt_vector, .mode = lvt_modes::PERIODIC, .mask = 0x0 });
    uint32_t init_count = uint64_t(ticks_per_second(LAPIC_DIVISOR)) * period_in_ms / 1000ul;
    write_to_register(LAPIC_INIT_COUNT, init_count);

    enable_interrupts();
}

void lapic_test_tools::global_apic_disable()
{
    auto msr_val{ rdmsr(msr::IA32_APIC_BASE) };
    msr_val &= ~(APIC_GLOBAL_ENABLED_MASK << APIC_GLOBAL_ENABLED_SHIFT);
    wrmsr(msr::IA32_APIC_BASE, msr_val);
}

void lapic_test_tools::global_apic_enable()
{
    auto msr_val{ rdmsr(msr::IA32_APIC_BASE) };
    msr_val |= (APIC_GLOBAL_ENABLED_MASK << APIC_GLOBAL_ENABLED_SHIFT);
    wrmsr(msr::IA32_APIC_BASE, msr_val);
}

bool lapic_test_tools::global_apic_enabled()
{
    return rdmsr(msr::IA32_APIC_BASE) & (APIC_GLOBAL_ENABLED_MASK << APIC_GLOBAL_ENABLED_SHIFT);
}

void lapic_test_tools::software_apic_disable()
{
    auto svr = read_from_register(LAPIC_SVR);
    svr &= ~(SVR_ENABLED_MASK << SVR_ENABLED_SHIFT);
    write_to_register(LAPIC_SVR, svr);
}

void lapic_test_tools::software_apic_enable()
{
    write_to_register(LAPIC_SVR, read_from_register(LAPIC_SVR) | (SVR_ENABLED_MASK << SVR_ENABLED_SHIFT));
}

[[nodiscard]] bool lapic_test_tools::software_apic_enabled()
{
    return (read_from_register(LAPIC_SVR) & (SVR_ENABLED_MASK << SVR_ENABLED_SHIFT)) != 0u;
}

void lapic_test_tools::send_self_ipi(uint8_t vector, dest_sh sh, dest_mode dest, dlv_mode dlv)
{
    // Avoid invalid combinations of delivery modes and vectors according to Intel SDM, Vol. 3, 10.6.1.
    // Vector is ignored for NMI and points to start routine for STARTUP.
    if (dlv == FIXED or dlv == LOWEST) {
        // don't use reserved vectors
        assert(vector == 2 or vector >= 32);
        assert(vector != NMI);

        // don't use spurious vector
        [[maybe_unused]] uint8_t spurious_vector{
            static_cast<uint8_t>(read_from_register(LAPIC_SVR) & SVR_VECTOR_MASK)
        };
        assert(vector != spurious_vector);
    }
    else if (dlv == SMI or dlv == INIT) {
        // vector field must be programmed to 0 for future compatibility
        assert(vector == 0);
    }

    wait_until_ready_for_ipi();

    uint64_t icr_entry = 0;

    icr_entry |= dlv << ICR_DLV_MODE_SHIFT;
    icr_entry |= dest << ICR_DEST_MODE_SHIFT;
    icr_entry |= level::ASSERT << ICR_LEVEL_SHIFT;
    icr_entry |= sh << ICR_DEST_SH_SHIFT;

    if (sh == dest_sh::NO_SH) {
        uint32_t apic_id = (read_from_register(LAPIC_ID) >> LAPIC_ID_SHIFT) & LAPIC_ID_MASK;
        icr_entry |= apic_id << ICR_DEST_SHIFT;
    }

    icr_entry |= vector;

    write_to_register(LAPIC_ICR_HIGH, icr_entry >> 32);
    write_to_register(LAPIC_ICR_LOW, icr_entry);

    wait_until_ready_for_ipi();
}

bool lapic_test_tools::check_irr(uint8_t vector)
{
    // Each IRR register holds the bits for 32 vectors and the individual
    // registers are 16 bytes apart.
    uintptr_t irr_address{ IRR_0_31 + 16 * (vector / 32) };
    size_t irr_bit{ 1u << (vector % 32) };

    return read_from_register(irr_address) & irr_bit;
}
