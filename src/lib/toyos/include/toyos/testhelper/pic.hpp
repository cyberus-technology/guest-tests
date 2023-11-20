/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "../x86/x86asm.hpp"
#include <toyos/util/interval.hpp>

/**
 * Driver for the Intel 8259 Programmable Interrupt Controller (PIC).
 * In today's hardware (and for many decades now), the PIC of a board is
 * manufactured in cascading mode where two 8259 are chained together. This
 * driver abstracts over the combined unit they form.
 */
class pic
{
    static constexpr uint16_t MASTER_CMD{ 0x20 };
    static constexpr uint16_t SLAVE_CMD{ 0xa0 };
    static constexpr uint16_t MASTER_DATA{ 0x21 };
    static constexpr uint16_t SLAVE_DATA{ 0xa1 };

    static constexpr uint8_t EOI{ 0x20 };
    static constexpr uint8_t ICW_INIT{ 0x10 };
    static constexpr uint8_t ICW_ICW4{ 0x01 };
    static constexpr uint8_t ICW_8086{ 0x01 };
    static constexpr uint8_t ICW_IRR{ 0x0a };
    static constexpr uint8_t ICW_ISR{ 0x0b };

    /**
     * Tells that the EOI belongs to the special vector and not the highest
     * vector.
     *
     * Source: "Figure 8. Operation Command Word Format" in spec.
     */
    static constexpr uint8_t SPECIFIC_EOI_FLAGS{ 0x60 };

 public:
    /*
     * Logical number of PINs the PIC provides. Only n-1 are usable, as one
     * pin is used for chaining the master to the slave.
     */
    static constexpr uint16_t PINS{ 16 };
    static constexpr uint16_t PINS_PER_PIC{ PINS / 2 };

    static constexpr uint8_t CASCADE_IRQ{ 2 };
    static constexpr uint8_t SPURIOUS_IRQ{ 7 };

    explicit pic(uint8_t vector_base)
        : vector_base_(vector_base)
    {
        if (not math::is_aligned(vector_base, 3)) {
            PANIC("Base vector has to be aligned to 8! Is=%x", vector_base);
        }

        // Mask all interrupts
        outb(MASTER_DATA, 0xff);
        outb(SLAVE_DATA, 0xff);

        outb(MASTER_CMD, ICW_INIT | ICW_ICW4);
        outb(SLAVE_CMD, ICW_INIT | ICW_ICW4);

        outb(MASTER_DATA, vector_base_);
        outb(SLAVE_DATA, vector_base_ + PINS_PER_PIC);

        outb(MASTER_DATA, 1u << CASCADE_IRQ);
        outb(SLAVE_DATA, CASCADE_IRQ);

        outb(MASTER_DATA, ICW_8086);
        outb(SLAVE_DATA, ICW_8086);

        mask_all();
    }

    [[nodiscard]] bool is_pic_vector(uint8_t vec) const
    {
        return cbl::interval::from_size(vector_base_, PINS).contains(vec);
    }

    [[nodiscard]] bool is_master_vector(uint8_t vec) const
    {
        return cbl::interval::from_size(vector_base_, PINS_PER_PIC).contains(vec);
    }

    [[nodiscard]] bool is_slave_vector(uint8_t vec) const
    {
        return cbl::interval::from_size(vector_base_ + PINS_PER_PIC, PINS_PER_PIC).contains(vec);
    }

    uint16_t get_irq_reg(uint8_t icw_reg) const
    {
        outb(MASTER_CMD, icw_reg);
        outb(SLAVE_CMD, icw_reg);
        return static_cast<uint16_t>(inb(SLAVE_CMD)) << 8 | inb(MASTER_CMD);
    }

    uint16_t get_irr() const
    {
        return get_irq_reg(ICW_IRR);
    }

    uint16_t get_isr() const
    {
        return get_irq_reg(ICW_ISR);
    }

    [[nodiscard]] bool is_spurious(uint8_t vec) const
    {
        if (not is_pic_vector(vec)) {
            return false;
        }

        uint8_t irq = (vec - vector_base_);
        if (irq != SPURIOUS_IRQ and irq != SPURIOUS_IRQ + PINS_PER_PIC) {
            return false;
        }

        return not(get_isr() & (1u << irq));
    }

    void mask(uint8_t vec) const
    {
        if (is_master_vector(vec)) {
            auto pin = vec - vector_base_;
            outb(MASTER_DATA, inb(MASTER_DATA) | (1u << pin));
        }

        if (is_slave_vector(vec)) {
            auto pin = vec - vector_base_ - PINS_PER_PIC;
            outb(SLAVE_DATA, inb(SLAVE_DATA) | (1u << pin));
        }
    }

    void unmask(uint8_t vec) const
    {
        if (is_master_vector(vec)) {
            auto pin = vec - vector_base_;
            outb(MASTER_DATA, inb(MASTER_DATA) & ~(1u << pin));
        }

        if (is_slave_vector(vec)) {
            auto pin = vec - vector_base_ - PINS_PER_PIC;
            outb(SLAVE_DATA, inb(SLAVE_DATA) & ~(1u << pin));
            unmask(CASCADE_IRQ);
        }
    }

    static void mask_all()
    {
        outb(MASTER_DATA, 0xff);
        outb(SLAVE_DATA, 0xff);
    }

    static void unmask_all()
    {
        outb(MASTER_DATA, 0);
        outb(SLAVE_DATA, 0);
    }

    [[nodiscard]] bool vector_in_irr(uint8_t vec) const
    {
        uint16_t pin = vec - vector_base_;
        return ((get_irr() >> pin) & 1) == 1;
    }

    /**
     * Sends an EOI for the highest interrupt.
     */
    bool eoi() const
    {
        auto maybe_isr_vec = highest_pending_isr_vec();
        if (!maybe_isr_vec.has_value()) {
            return false;
        }
        if (is_slave_vector(maybe_isr_vec.value())) {
            outb(SLAVE_CMD, EOI);
        }
        outb(MASTER_CMD, EOI);

        return true;
    }

    /// Returns the effective vector of the highest IRQ in ISR.
    [[nodiscard]] std::optional<uint16_t> highest_pending_isr_vec() const
    {
        auto maybe_pin = highest_pending_isr_pin();
        // C++ has no .map()
        // TODO: wait for C++23: https://en.cppreference.com/w/cpp/utility/optional/transform
        if (maybe_pin.has_value()) {
            return { maybe_pin.value() + vector_base_ };
        }
        return std::nullopt;
    }

 private:
    uint8_t vector_base_;

    /**
     * Returns the position of the highest bit that is set in the provided val.
     */
    static std::optional<uint16_t> position_of_highest_bit(uint16_t val)
    {
        if (val == 0) {
            return std::nullopt;
        }
        for (uint16_t i = PINS; i > 0; i--) {
            auto pos = i - 1;
            if (((val >> pos) & 1) == 1) {
                return { pos };
            }
        }
        __UNREACHED__
    }

    /// Returns the pin number of the highest IRQ in ISR.
    std::optional<uint16_t> highest_pending_isr_pin() const
    {
        return position_of_highest_bit(get_isr());
    }
};
