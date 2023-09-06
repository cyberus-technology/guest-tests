/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/x86/arch.hpp>
#include <cbl/cast_helpers.hpp>
#include "ioapic.hpp"

/**
 * Simple HPET abstraction.
 *
 * It is constructed directly at the MMIO address,
 * which defaults to 0xfed00000.
 *
 * Supported features:
 *   - Read/Write main counter
 *   - Configure individual timers to MSI or legacy interrupts
 *   - Legacy Replacement Route
 */
struct __PACKED__ hpet
{
    static constexpr uintptr_t DEFAULT_ADDRESS {0xfed00000}; ///< Default MMIO address on our systems.

    /// Returns a pointer to the HPET with the given base.
    static hpet* get(uintptr_t base = DEFAULT_ADDRESS) { return num_to_ptr<hpet>(base); }

    /**
     * Timer configuration helper.
     *
     * This can be used to configure a single
     * timer (interrupt delivery, comparator).
     */
    struct __PACKED__ timer
    {
    public:
        /// Trigger mode of the interrupt
        enum class trigger : uint8_t
        {
            EDGE = 0,  ///< Trigger on edge
            LEVEL = 1, ///< Trigger on level
        };

        bool fsb_capable() const { return cfg & TMR_CAP_FSB; }    ///< Returns whether the timer can generate MSIs.
        bool fsb_enabled() const { return cfg & TMR_FSB_ENABLE; } ///< Returns whether MSI delivery is enabled.
        bool periodic() const { return cfg & TMR_INT_PERIODIC; }  ///< Returns whether the timer is set to periodic.

        /// Returns the bitmask of available I/O APIC interrupts.
        uint32_t available_gsis() const
        {
            // The HPET inside VirtualBox reports more available gsis than there are pins at the IOAPIC. This can lead
            // to errors if we try to use a too high gsi. Thus, we mask the available gsis here to avoid these errors.
            const ioapic ioapic;
            return route_cap & math::mask(ioapic.max_irt() + 1);
        }

        /// Returns the configured I/O APIC interrupt.
        uint8_t ioapic_gsi() const { return (cfg >> TMR_ROUTE_SHIFT) & math::mask(TMR_ROUTE_BITS); }

        /// Sets the interrupt to the given trigger mode.
        void trigger_mode(trigger trg)
        {
            cfg &= ~TMR_INT_TRIGGER;
            cfg |= to_underlying(trg) * TMR_INT_TRIGGER;
        }

        /// Sets the interrupt enable flag to the value of i.
        void int_enabled(bool i) { configure_field(&cfg, TMR_INT_ENABLE, i); }

        /// Sets the periodic flag to the value of p.
        void periodic(bool p) { configure_field(&cfg, TMR_INT_PERIODIC, p); }

        /// Sets the MSI flag to the value of e.
        void fsb_enabled(bool e) { configure_field(&cfg, TMR_FSB_ENABLE, e); }

        /// Sets the I/O apic interrupt for this timer.
        void ioapic_gsi(uint8_t gsi)
        {
            cfg &= ~math::mask(TMR_ROUTE_BITS, TMR_ROUTE_SHIFT);
            cfg |= (gsi & math::mask(TMR_ROUTE_BITS)) << TMR_ROUTE_SHIFT;
        }

        /// Sets the comparator value to val.
        void comparator(uint64_t val)
        {
            comp_hi = val >> 32;
            comp_lo = val;
        }

        /// Sets the MSI configuration with the given addr and data.
        void msi_config(uint32_t addr, uint32_t data)
        {
            msi_addr = addr;
            msi_data = data;
        }

    private:
        enum timer_bits
        {
            TMR_INT_TRIGGER = 1u << 1,
            TMR_INT_ENABLE = 1u << 2,
            TMR_INT_PERIODIC = 1u << 3,
            TMR_CAP_PERIODIC = 1u << 4,
            TMR_SIZE = 1u << 5,
            TMR_32BIT = 1u << 8,
            TMR_FSB_ENABLE = 1u << 14,
            TMR_CAP_FSB = 1u << 15,

            TMR_ROUTE_BITS = 5,
            TMR_ROUTE_SHIFT = 9,
        };
        volatile uint32_t cfg;       ///< Config and capabilities register (0x100 + N * 0x20)
        volatile uint32_t route_cap; ///< Routing capabilities (possible I/O APIC pins)

        volatile uint32_t comp_lo; ///< Lower 32 bits of the comparator value.
        volatile uint32_t comp_hi; ///< Upper 32 bits of the comparator value.

        volatile uint32_t msi_data; ///< MSI data configuration (delivery mode, vector).
        volatile uint32_t msi_addr; ///< MSI address configuration (destination).
    };

    /// Reads the vendor ID of this HPET device.
    uint16_t vendor() const { return extract(cap_id, CAP_VENDOR_BITS, CAP_VENDOR_SHIFT); }

    /// Reads the number of available timers.
    size_t timer_count() const { return extract(cap_id, CAP_TMR_COUNT_BITS, CAP_TMR_COUNT_SHIFT); }

    /// Globally enables/disables the HPET device according to e.
    void enabled(bool e) { configure_field(&cfg, CFG_ENABLED, e); }

    /// Sets the legacy replacement route option to the value of l.
    void legacy_enabled(bool l) { configure_field(&cfg, CFG_LEGACY, l); }

    /// Returns a pointer to the timer n.
    timer* get_timer(size_t n) const { return num_to_ptr<timer>(ptr_to_num(this) + Tn_BASE + n * Tn_CFG_OFFSET); }

    /// Calculates the number of main counter ticks to form the given amount of microseconds.
    uint64_t microseconds_to_ticks(uint64_t us) const
    {
        // Period is one tick in femtoseconds (10^-15 seconds). We divide 10^9 by the period to
        // get ticks per microsecond and then multiply.
        return (1000000000ull / period) * us;
    }

    /// Calculates the number of main counter ticks to form the given amount of milliseconds.
    uint64_t milliseconds_to_ticks(uint64_t ms) const { return microseconds_to_ticks(1000 * ms); }

    /// Reads the main counter value.
    uint64_t main_counter() const
    {
        uint64_t main_hi;
        uint64_t main_lo;

        // Make sure the high 32-bit value is not overflowing right when we read it.
        do {
            main_hi = main_counter_hi;
            main_lo = main_counter_lo;
        } while (main_hi != main_counter_hi);

        return (main_hi << 32) | main_lo;
    }

    /// Sets the main counter value to new_cnt.
    void main_counter(uint64_t new_cnt)
    {
        main_counter_hi = new_cnt >> 32;
        main_counter_lo = new_cnt;
    }

    /// Returns whether the given timer has an active IRQ. Only applies to level-triggered configurations.
    bool interrupt_active(uint8_t timer_no) const { return int_status & (1u << timer_no); }

    /// Clears the active IRQ flag for the given timer.
    void clear_irq(uint8_t timer_no) { int_status |= (1u << timer_no); }

protected:
    volatile uint32_t cap_id;
    volatile uint32_t period;

    volatile uint64_t resvd1;

    volatile uint32_t cfg;
    volatile uint32_t resvd2;

    volatile uint64_t resvd3;

    volatile uint32_t int_status;
    volatile uint32_t resvd4;

    uint64_t resvd5[25];

    volatile uint32_t main_counter_lo;
    volatile uint32_t main_counter_hi;

    enum
    {
        CAP_VENDOR_SHIFT = 16,
        CAP_VENDOR_BITS = 16,
        CAP_TMR_COUNT_SHIFT = 8,
        CAP_TMR_COUNT_BITS = 4,

        CFG_ENABLED = 1u << 0,
        CFG_LEGACY = 1u << 1,

        Tn_BASE = 0x100,
        Tn_CFG_OFFSET = 0x020,
    };

    static void configure_field(volatile uint32_t* field, uint32_t mask, bool set)
    {
        if (set) {
            *field |= mask;
        } else {
            *field &= ~mask;
        }
    }

    static uint32_t extract(uint32_t val, size_t bits, size_t shift)
    {
        return (val & math::mask(bits, shift)) >> shift;
    }
};
