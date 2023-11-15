/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/util/cast_helpers.hpp>
#include <toyos/util/interval.hpp>
#include <toyos/util/math.hpp>
#include <toyos/util/trace.hpp>
#include <toyos/x86/arch.hpp>

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
    static constexpr cbl::interval VALID_MAX_IRTS{ 23, 120 };

    static constexpr uintptr_t DEFAULT_BASE{ 0xfec00000 };  ///< Standard base MMIO address of the primary I/O APIC.

    static constexpr size_t REG_SELECT{ 0x00 };  ///< Offset of register select.
    static constexpr size_t REG_DATA{ 0x10 };    ///< Offset of window.

    /// I/O APIC register indexes
    enum class reg : uint32_t
    {
        ID = 0x00,       ///< I/O APIC ID
        VERSION = 0x01,  ///< I/O APIC Version (not to be trusted :()
        IRT0 = 0x10,     ///< First redirection table entry
    };

    /// Shift constants within registers
    enum class shift : uint8_t
    {
        NONE = 0,      ///< Can be used to read the whole register
        ID = 24,       ///< Location of the actual ID within the ID register.
        MAX_IRT = 16,  ///< Location of the maximum redirection entry index within the VERSION register.
    };

 public:
    /// Constructs an I/O APIC helper with the given base address.
    explicit ioapic(uintptr_t base_ = DEFAULT_BASE)
        : base(base_) {}

    /// Validates if the values read from the version register make sense.
    bool validate() const
    {
        return read(reg::VERSION) != ~0u and VALID_MAX_IRTS.contains(max_irt());
    }

    uint8_t id() const
    {
        return read(reg::ID, shift::ID);
    }  ///< Reads the I/O APIC ID.
    uint8_t version() const
    {
        return read(reg::VERSION);
    }  ///< Reads the I/O APIC version.
    uint8_t max_irt() const
    {
        return read(reg::VERSION, shift::MAX_IRT);
    }  ///< Reads the maximum redirection entry.

    /**
     * Redirection entry abstraction.
     *
     * This helper structure can be used to interpret redirection entries and
     * manipulate it.
     *
     * It can also be used to construct a new one for given values.
     *
     * Note that the entry is a standalone copy. Modifications are not
     * applied immediately to the IoApic in hardware. The intended use is to get
     * or construct an entry, and then update the I/O APIC using the set_irt()
     * method.
     */
    struct redirection_entry
    {
        /// Trigger mode of the interrupt
        enum class trigger_mode : uint8_t
        {
            EDGE = 0,   ///< Trigger on edge
            LEVEL = 1,  ///< Trigger on level
        };

        /// Delivery mode
        enum class dlv_mode : uint8_t
        {
            FIXED = 0,   ///< Fixed delivery mode
            NMI = 4,     ///< NMI delivery mode
            EXTINT = 7,  ///< Ext Int delivery mode (PIC)
        };

        /// Pin polarity.
        enum class pin_polarity : uint8_t
        {
            ACTIVE_HIGH = 0,  ///< Polarity high.
            ACTIVE_LOW = 1,   ///< Polarity low.
        };

        /// Destination mode. This field determines the interpretation of the
        /// Destination field.
        enum class dst_mode : uint8_t
        {
            PHYSICAL = 0,  ///< Physical destination mode.
            LOGICAL = 1,   ///< Logical destination mode.
        };

        /// Definitions of the individual components of the bitfield
        enum bitmasks
        {
            VECTOR_BITS = 8,   ///< Number of bits that form a set_vector
            VECTOR_SHIFT = 0,  ///< Position of the set_vector

            DLV_MODE_BITS = 3,   ///< Number of bits that form a delivery mode
            DLV_MODE_SHIFT = 8,  ///< Position of the delivery mode

            DEST_MODE_SHIFT = 1u << 11,     ///< Position of the destination mode
            SEND_PENDING = 1u << 12,        ///< Send pending bit
            PIN_POLARITY_SHIFT = 1u << 13,  ///< Position of the pin polarity bit
            REMOTE_IRR = 1u << 14,          ///< Remote IRR bit
            TRIGGER_MODE = 1u << 15,        ///< Trigger mode bit
            MASKED = 1u << 16,              ///< Mask bit

            DEST_BITS = 8,    ///< Number of bits that form a destination
            DEST_SHIFT = 56,  ///< Position of the destination
        };

        /// Constructs a redirection entry for a given pin and raw value.
        /// The entry uses physical destination mode.
        redirection_entry(uint8_t pin, uint64_t value)
            : index(pin), raw(value) {}

        /// Constructs a redirection entry and unmasks it but doesn't commit it
        /// to the hardware. By default, the pin polarity is ACTIVE_HIGH and
        /// the destination mode is set to PHYSICAL.
        ///
        /// Guidelines on how to specify the parameters:
        /// - Delivery method NMI: The vector field is ignored as this cannot be
        ///   anything except for vector 2.
        /// - Default behaviour on the hardware is that IRQ 0-15 (exceptions)
        ///   are edge-high, whereas IRQ 16..IOAPIC_MAX_PIN are level-low.
        ///   This is also what the original NOVA hypervisor configured
        ///   (https://github.com/udosteinberg/NOVA/blob/master/src/gsi.cpp#L42).
        ///
        ///   This is not always the case, the correct trigger mode and
        ///   polarity depends on the ACPI PCI Routing table (_PRT), but it
        ///   requires a full ACPICA stack to parse the DSDT. In addition,
        ///   there may be ACPI source overrides which can override the _PRT.
        ///   And then there's the MP Table (legacy) which also defines
        ///   polarity and trigger mode. You need to combine all these
        ///   information to find the effective interrupt routing information.
        ///
        ///   More reference: https://people.freebsd.org/~jhb/papers/bsdcan/2007/article/node5.html
        redirection_entry(
            uint8_t pin /* also the index of the entry */,
            uint8_t vec,
            uint8_t dst,
            dlv_mode dlv_mode_,
            trigger_mode trigger_mode_,
            pin_polarity pin_polarity_ = pin_polarity::ACTIVE_HIGH,
            dst_mode dst_mode_ = dst_mode::PHYSICAL)
            : index(pin)
        {
            set_vector(vec);
            set_delivery_mode(dlv_mode_);
            set_dest(dst);
            set_trigger_mode(trigger_mode_);
            set_dst_mode(dst_mode_);
            set_pin_polarity(pin_polarity_);
            unmask();
        }

        /// Returns the delivery mode of this redirection entry.
        dlv_mode delivery_mode() const
        {
            return dlv_mode((raw >> DLV_MODE_SHIFT) & DLV_MODE_BITS);
        }

        uint8_t vector() const
        {
            return raw;
        }  ///< Returns the interrupt set_vector.
        bool active_low() const
        {
            return raw & PIN_POLARITY_SHIFT;
        }  ///< Returns whether the polarity is active low.
        bool remote_irr() const
        {
            return raw & REMOTE_IRR;
        }  ///< Returns whether REMOTE_IRR is set.
        bool masked() const
        {
            return raw & MASKED;
        }  ///< Returns whether the entry is masked.
        uint8_t dest() const
        {
            return raw >> DEST_SHIFT;
        }  ///< Returns the destination.

        /// Sets the vector.
        void set_vector(uint8_t vec)
        {
            raw &= ~math::mask(VECTOR_BITS, VECTOR_SHIFT);
            raw |= vec << VECTOR_SHIFT;
        }

        /// Sets the delivery mode.
        void set_delivery_mode(dlv_mode dlv)
        {
            raw &= ~math::mask(DLV_MODE_BITS, DLV_MODE_SHIFT);
            raw |= (to_underlying(dlv) & math::mask(DLV_MODE_BITS)) << DLV_MODE_SHIFT;
        }

        /// Sets the destination.
        void set_dest(uint8_t dst)
        {
            raw &= ~math::mask(DEST_BITS, DEST_SHIFT);
            raw |= static_cast<uint64_t>(dst) << DEST_SHIFT;
        }

        /// Sets the trigger mode.
        void set_trigger_mode(trigger_mode trg)
        {
            raw &= ~TRIGGER_MODE;
            raw |= to_underlying(trg) * TRIGGER_MODE;
        }

        /// Sets the destination mode bit.
        void set_dst_mode(dst_mode mode)
        {
            raw &= ~bitmasks::DEST_MODE_SHIFT;
            raw |= to_underlying(mode) * DEST_MODE_SHIFT;
        }

        /// Sets the destination mode bit.
        void set_pin_polarity(pin_polarity polarity)
        {
            raw &= ~bitmasks::PIN_POLARITY_SHIFT;
            raw |= to_underlying(polarity) * PIN_POLARITY_SHIFT;
        }

        void mask()
        {
            raw |= MASKED;
        }  ///< Masks the entry.
        void unmask()
        {
            raw &= ~MASKED;
        }  ///< Unmasks the entry.

        void set_active_low()
        {
            raw |= PIN_POLARITY_SHIFT;
        }  ///< Sets the polarity to active low.
        void set_active_high()
        {
            raw &= ~PIN_POLARITY_SHIFT;
        }  ///< Sets the polarity to active high.

        uint8_t index{ 0 };  ///< The redirection entry index.
        uint64_t raw{ 0 };   ///< The raw 64-bit value of this entry.
    };

    /// Returns a copy of the redirection entry at `idx`.
    [[nodiscard]] redirection_entry get_irt(uint8_t idx) const
    {
        auto lo = read(reg(to_underlying(reg::IRT0) + idx * 2));
        auto hi = read(reg(to_underlying(reg::IRT0) + idx * 2 + 1));
        return { idx, static_cast<uint64_t>(hi) << 32 | lo };
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
