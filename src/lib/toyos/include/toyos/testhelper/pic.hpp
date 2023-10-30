/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "../x86/x86asm.hpp"
#include <toyos/util/interval.hpp>

/**
 * Driver for the Intel 8259 Programmable Interrupt Controller (PIC).
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

    static constexpr uint8_t CASCADE_IRQ{ 2 };
    static constexpr uint8_t SPURIOUS_IRQ{ 7 };

    /**
     * Tells that the EOI belongs to the special vector and not the highest
     * vector.
     *
     * Source: "Figure 8. Operation Command Word Format" in spec.
     */
    static constexpr uint8_t SPECIFIC_EOI_FLAGS{ 0x60 };

 public:
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
        outb(SLAVE_DATA, vector_base_ + 8);

        outb(MASTER_DATA, 1u << CASCADE_IRQ);
        outb(SLAVE_DATA, CASCADE_IRQ);

        outb(MASTER_DATA, ICW_8086);
        outb(SLAVE_DATA, ICW_8086);

        // Mask all interrupts
        outb(MASTER_DATA, 0xff);
        outb(SLAVE_DATA, 0xff);
    }

    bool is_pic_vector(uint8_t vec)
    {
        return cbl::interval::from_size(vector_base_, 16).contains(vec);
    }

    bool is_master_vector(uint8_t vec)
    {
        return cbl::interval::from_size(vector_base_, 8).contains(vec);
    }

    bool is_slave_vector(uint8_t vec)
    {
        return cbl::interval::from_size(vector_base_ + 8, 8).contains(vec);
    }

    uint16_t get_irq_reg(uint8_t icw_reg)
    {
        outb(MASTER_CMD, icw_reg);
        outb(SLAVE_CMD, icw_reg);
        return static_cast<uint16_t>(inb(SLAVE_CMD)) << 8 | inb(MASTER_CMD);
    }

    uint16_t get_irr()
    {
        return get_irq_reg(ICW_IRR);
    }

    uint16_t get_isr()
    {
        return get_irq_reg(ICW_ISR);
    }

    bool is_spurious(uint8_t vec)
    {
        if (not is_pic_vector(vec)) {
            return false;
        }

        uint8_t irq = (vec - vector_base_);
        if (irq != SPURIOUS_IRQ and irq != SPURIOUS_IRQ + 8) {
            return false;
        }

        return not(get_isr() & (1u << irq));
    }

    void mask(uint8_t vec)
    {
        if (is_master_vector(vec)) {
            outb(MASTER_DATA, inb(MASTER_DATA) | (1u << (vec - vector_base_)));
        }

        if (is_slave_vector(vec)) {
            outb(SLAVE_DATA, inb(SLAVE_DATA) | (1u << (vec - vector_base_ - 8)));
        }
    }

    void unmask(uint8_t vec)
    {
        if (is_master_vector(vec)) {
            outb(MASTER_DATA, inb(MASTER_DATA) & ~(1u << (vec - vector_base_)));
        }

        if (is_slave_vector(vec)) {
            outb(SLAVE_DATA, inb(SLAVE_DATA) & ~(1u << (vec - vector_base_ - 8)));
            unmask(CASCADE_IRQ);
        }
    }

    /**
     * Sends an EOI for the highest interrupt.
     */
    bool eoi(uint8_t vec)
    {
        if (is_pic_vector(vec)) {
            if (is_slave_vector(vec)) {
                outb(SLAVE_CMD, EOI);
            }

            outb(MASTER_CMD, EOI);
            return true;
        }
        return false;
    }

 private:
    uint8_t vector_base_;
};
