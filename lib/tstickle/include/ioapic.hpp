/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.hpp>
#include <cbl/cast_helpers.hpp>
#include "cbl/math.hpp"

#include <cbl/interval.hpp>

/**
 * Simple I/O APIC abstraction.
 *
 * It can be used to configure redirection entries
 * to deliver interrupts that come in via I/O APIC
 * pins.
 *
 * The register set of I/O APIC is accessed via
 * a pair of MMIO registers, "register select" and
 * "window", or index/data, respectively.
 */
class ioapic
{
    /**
     * Used to validate we actually found an I/O APIC at the base address.
     * We currently only see primary I/O APICs with 24-120 pins,
     * which translates into a maximum IRT index of 23-119.
     */
    static constexpr cbl::interval VALID_MAX_IRTS {23, 120};

    static constexpr uintptr_t DEFAULT_BASE {0xfec00000}; ///< Standard base MMIO address of the primary I/O APIC.

    static constexpr size_t REG_SELECT {0x00}; ///< Offset of register select.
    static constexpr size_t REG_DATA {0x10};   ///< Offset of window.

    /// I/O APIC register indexes
    enum class reg : uint32_t
    {
        ID = 0x00,      ///< I/O APIC ID
        VERSION = 0x01, ///< I/O APIC Version (not to be trusted :()
        IRT0 = 0x10,    ///< First redirection table entry
    };

    /// Shift constants within registers
    enum class shift : uint8_t
    {
        NONE = 0,     ///< Can be used to read the whole register
        ID = 24,      ///< Location of the actual ID within the ID register.
        MAX_IRT = 16, ///< Location of the maximum redirection entry index within the VERSION register.
    };

public:
    /// Constructs an I/O APIC helper with the given base address.
    explicit ioapic(uintptr_t base_ = DEFAULT_BASE) : base(base_) {}

    /// Validates if the values read from the version register make sense.
    bool validate() const { return read(reg::VERSION) != ~0u and VALID_MAX_IRTS.contains(max_irt()); }

    uint8_t id() const { return read(reg::ID, shift::ID); }                ///< Reads the I/O APIC ID.
    uint8_t version() const { return read(reg::VERSION); }                 ///< Reads the I/O APIC version.
    uint8_t max_irt() const { return read(reg::VERSION, shift::MAX_IRT); } ///< Reads the maximum redirection entry.

    /**
     * Redirection entry abstraction.
     *
     * This helper structure can be used to interpret
     * redirection entries and manipulate it.
     *
     * It can also be used to construct a new one for
     * given values. Currently, the delivery mode is always
     * FIXED, the polarity is always active low, and the entry
     * is initially unmasked.
     *
     * Note that the entry is a copy. Modifications are not
     * applied immediately. The intended use is to get
     * or construct an entry, and then update the I/O APIC
     * using the set_irt() method.
     */
    struct redirection_entry
    {
        /// Trigger mode of the interrupt
        enum class trigger : uint8_t
        {
            EDGE = 0,  ///< Trigger on edge
            LEVEL = 1, ///< Trigger on level
        };

        /// Delivery mode
        enum class dlv_mode : uint8_t
        {
            FIXED = 0, ///< Fixed delivery mode
            NMI = 4,   ///< NMI delivery mode
        };

        /// Definitions of the individual components of the bitfield
        enum bitmasks
        {
            VECTOR_BITS = 8,  ///< Number of bits that form a vector
            VECTOR_SHIFT = 0, ///< Position of the vector

            DLV_MODE_BITS = 3,  ///< Number of bits that form a delivery mode
            DLV_MODE_SHIFT = 8, ///< Position of the delivery mode

            DEST_LOGICAL = 1u << 11, ///< Logical destination bit
            SEND_PENDING = 1u << 12, ///< Send pending bit
            POLARITY_LOW = 1u << 13, ///< Active low bit
            REMOTE_IRR = 1u << 14,   ///< Remote IRR bit
            TRIGGER_MODE = 1u << 15, ///< Trigger mode bit
            MASKED = 1u << 16,       ///< Mask bit

            DEST_BITS = 8,   ///< Number of bits that form a destination
            DEST_SHIFT = 56, ///< Position of the destination
        };

        /// Constructs a redirection entry for a given pin and raw value.
        redirection_entry(uint8_t pin, uint64_t value) : index(pin), raw(value) {}

        /// Constructs a redirection entry for a given pin, vector, destination, and edge/level configuration.
        redirection_entry(uint8_t pin, uint8_t vec, uint8_t dst, trigger trg) : index(pin)
        {
            vector(vec);
            delivery_mode(dlv_mode::FIXED);
            dest(dst);
            trigger_mode(trg);
            set_active_low();
            unmask();
        }

        /// Returns the delivery mode of this redirection entry.
        dlv_mode delivery_mode() const { return dlv_mode((raw >> DLV_MODE_SHIFT) & DLV_MODE_BITS); }

        uint8_t vector() const { return raw; }                   ///< Returns the interrupt vector.
        bool logical_dest() const { return raw & DEST_LOGICAL; } ///< Returns whether the destination is logical.
        bool active_low() const { return raw & POLARITY_LOW; }   ///< Returns whether the polarity is active low.
        bool remote_irr() const { return raw & REMOTE_IRR; }     ///< Returns whether REMOTE_IRR is set.
        bool masked() const { return raw & MASKED; }             ///< Returns whether the entry is masked.
        uint8_t dest() const { return raw >> DEST_SHIFT; }       ///< Returns the destination.

        /// Returns the trigger mode of this redirection entry.
        trigger trigger_mode() const { return trigger(raw & TRIGGER_MODE); }

        /// Sets the vector to vec.
        void vector(uint8_t vec)
        {
            raw &= ~math::mask(VECTOR_BITS, VECTOR_SHIFT);
            raw |= vec << VECTOR_SHIFT;
        }

        /// Sets the delivery mode to dlv.
        void delivery_mode(dlv_mode dlv)
        {
            raw &= ~math::mask(DLV_MODE_BITS, DLV_MODE_SHIFT);
            raw |= (to_underlying(dlv) & math::mask(DLV_MODE_BITS)) << DLV_MODE_SHIFT;
        }

        /// Sets the destination to dst.
        void dest(uint8_t dst)
        {
            raw &= ~math::mask(DEST_BITS, DEST_SHIFT);
            raw |= static_cast<uint64_t>(dst) << DEST_SHIFT;
        }

        /// Sets the trigger mode to trg.
        void trigger_mode(trigger trg)
        {
            raw &= ~TRIGGER_MODE;
            raw |= to_underlying(trg) * TRIGGER_MODE;
        }

        void mask() { raw |= MASKED; }    ///< Masks the entry.
        void unmask() { raw &= ~MASKED; } ///< Unmasks the entry.

        void set_active_low() { raw |= POLARITY_LOW; }   ///< Sets the polarity to active low.
        void set_active_high() { raw &= ~POLARITY_LOW; } ///< Sets the polarity to active high.

        uint8_t index {0}; ///< The redirection entry index.
        uint64_t raw {0};  ///< The raw 64-bit value of this entry.
    };

    /// Returns a copy of the redirection entry idx.
    redirection_entry get_irt(uint8_t idx) const
    {
        auto lo = read(reg(to_underlying(reg::IRT0) + idx * 2));
        auto hi = read(reg(to_underlying(reg::IRT0) + idx * 2 + 1));
        return {idx, static_cast<uint64_t>(hi) << 32 | lo};
    }

    /// Updates the given redirection entry.
    void set_irt(const redirection_entry& entry) const
    {
        // First we mask the entry to prevent undefined interrupt behavior
        // Then we update the high part, and lastly the low part, which contains
        // the final mask bit state.
        auto reg_lo = reg(to_underlying(reg::IRT0) + entry.index * 2);
        auto reg_hi = reg(to_underlying(reg::IRT0) + entry.index * 2 + 1);
        write(reg_lo, read(reg_lo) | redirection_entry::MASKED);
        write(reg_hi, entry.raw >> 32);
        write(reg_lo, entry.raw & math::mask(32));
    }

private:
    uint32_t read(reg r, shift s = shift::NONE) const
    {
        *num_to_ptr<volatile uint32_t>(base + REG_SELECT) = to_underlying(r);
        return *num_to_ptr<volatile uint32_t>(base + REG_DATA) >> to_underlying(s);
    }

    void write(reg r, uint32_t val) const
    {
        *num_to_ptr<volatile uint32_t>(base + REG_SELECT) = to_underlying(r);
        *num_to_ptr<volatile uint32_t>(base + REG_DATA) = val;
    }

    uintptr_t base;
};
