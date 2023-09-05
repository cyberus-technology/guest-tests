/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <trace.hpp>

#include <math.hpp>
#include <x86asm.hpp>

/**
 * A driver for the programmable interval timer.
 * You can only choose the operating mode, everything else is always:
 *  channel:        channel 0
 *  access mode:    lowest byte then highest byte
 *  counter format: binary
 *
 *  The supported operating modes are:
 *      interrupt on terminal count (oneshot-mode)
 *      rate generator (periodic interrupts)
 *
 *  Everything else is not supported and will panic.
 */
class pit
{
    static constexpr uint16_t DATA0 {0x40};
    static constexpr uint16_t DATA1 {0x41};
    static constexpr uint16_t DATA2 {0x42};
    static constexpr uint16_t MODE {0x43};

    enum
    {
        FORMAT_BITS = 1,
        OPERATING_MODE_BITS = 3,
        ACCESS_MODE_BITS = 2,
        CHANNEL_BITS = 2,

        FORMAT_SHIFT = 0,
        OPERATING_MODE_SHIFT = 1,
        ACCESS_MODE_SHIFT = 4,
        CHANNEL_SHIFT = 6,

        FORMAT_MASK = math::mask(FORMAT_BITS, FORMAT_SHIFT),
        OPERATING_MODE_MASK = math::mask(OPERATING_MODE_BITS, OPERATING_MODE_SHIFT),
        ACCESS_MODE_MASK = math::mask(ACCESS_MODE_BITS, ACCESS_MODE_SHIFT),
        CHANNEL_MASK = math::mask(CHANNEL_BITS, CHANNEL_SHIFT),
    };

    /// The output channel the PIT will use
    enum class channel
    {
        CHANNEL_0 = 0, /// interrupt 0
        CHANNEL_1 = 1, /// was once used to refresh the DRAM, not usable anymore
        CHANNEL_2 = 2, /// connected to the PC speaker
        READ_BACK = 3, /// read-back command to read a status value for a selected channel
    };

    /// How to access the reload register
    enum class access_mode
    {
        LATCH_COUNT = 0, /// counter latch command to read the current count without inconsistencies
        LOBYTE_ONLY = 1, /// read/write only lowest 8 bit
        HIBYTE_ONLY = 2, /// read/write only highest 8 bit
        LO_HIBYTE = 3,   /// read/write lowest 8 bit, then highest 8 bit
    };

    /// determines if the pit channel operates in binary mode or bcd
    enum class counter_format
    {
        BINARY = 0, /// binary mode
        BCD = 1,    /// four digit bcd
    };

public:
    /// Determines how the pit works, e.g. creating one interrupt or periodic interrupts
    enum class operating_mode
    {
        INTERRUPT_ON_TERMINAL_COUNT = 0,     /// one interrupt after a given amount of time
        HARDWARE_RETRIGGERABLE_ONE_SHOT = 1, /// similar to mode 0, but cant be used for channels 0 or 1
        RATE_GENERATOR = 2,                  /// periodic interrupts
        SQUARE_WAVE_GENERATOR = 3,           /// similar to mode 2, but the signal will stay high/low
        SOFTWARE_TRIGGERED_STROBE = 4,       /// keeps creating interrupts when the counter hits 0
        HARDWARE_TRIGGERED_STROBE = 5,       /// similar to mode 4, but cant be used for channels 0 or 1
    };

    /**
     * Constructor configuring the PIT with the given values
     *
     * \param op_mode The operating mode to be used
     */
    pit(operating_mode op_mode) { set_operating_mode(op_mode); }

    /**
     * Sets the operating mode according to the given mode.
     *
     * \param mode The operating mode to be used
     */
    void set_operating_mode(operating_mode op_mode)
    {
        PANIC_UNLESS(op_mode == operating_mode::INTERRUPT_ON_TERMINAL_COUNT
                         || op_mode == operating_mode::RATE_GENERATOR,
                     "You chose an unsupported operating mode!");
        op_mode_ = op_mode;
        outb(MODE, create_control_word());
    }

    /**
     * Sets the counter according to the Channel (which determines the data-port) and the access mode.
     *
     * \param value The value you want to write to the counter
     */
    void set_counter(uint16_t value)
    {
        outb(data_port_, value & 0xff);
        outb(data_port_, (value >> 8) & 0xff);
    }

private:
    const channel channel_ {channel::CHANNEL_0};
    const access_mode acc_mode_ {access_mode::LO_HIBYTE};
    const counter_format format_ {counter_format::BINARY};
    const uint16_t data_port_ {DATA0};

    operating_mode op_mode_;

    /**
     * Creates the control word from the member variables.
     */
    uint8_t create_control_word()
    {
        uint8_t word = 0;
        set_bits(word, channel_, CHANNEL_SHIFT, CHANNEL_MASK);
        set_bits(word, acc_mode_, ACCESS_MODE_SHIFT, ACCESS_MODE_MASK);
        set_bits(word, op_mode_, OPERATING_MODE_SHIFT, OPERATING_MODE_MASK);
        set_bits(word, format_, FORMAT_SHIFT, FORMAT_MASK);
        return word;
    }

    template <class T>
    void set_bits(uint8_t& raw, T set_value, uint8_t set_shift, uint8_t clr_mask)
    {
        raw &= ~clr_mask;
        raw |= (static_cast<uint8_t>(set_value) << set_shift) & clr_mask;
    }
};
