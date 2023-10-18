/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <map>
#include <optional>

#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/x86/x86asm.hpp>

/*
 * X2APIC mode can only be disabled by disabling the APIC completely
 * (BASE MSR EXT=0 and EN=0).
 * Afterwards, the APIC needs to be enabled again. It is then set
 * to a state which does not fit the initial state which the BIOS has
 * set.
 * Examples:
 *            | BIOS   | Reset
 *      SVR   | 0x10f  | 0xff
 *      LINT0 | 0x1700 | 0x10000
 *      LINT1 | 0x400  | 0x10000
 *
 * Note that re-enabling XAPIC leaves the APIC software disabled (SVR register).
 * Several operations work although the APIC is disabled:
 *  - sending an IPI in XAPIC mode
 *  - switching to X2APIC mode
 *      - reading and writing TPR in X2APIC mode
 * But, a GPE is triggered when
 *  - switching to X2APIC mode and then
 *      - writing the SELF_IPI register
 *      - using ICR to trigger an IPI
 */
namespace x2apic_test_tools
{

    constexpr uint32_t X2APIC_ENABLED_MASK{ 1u };
    constexpr uint32_t X2APIC_ENABLED_SHIFT{ 10u };

    struct x2apic_msr
    {
        enum mode_t
        {
            READ = 1u << 0u,
            WRITE = 1u << 1u,
            RW = READ | WRITE,
        };

        enum width_t
        {
            BITS_32,
            BITS_64,
        };

        enum preserve_t
        {
            PRESERVED_ON_INIT,
            NOT_PRESERVED_ON_INIT,
            X2APIC_ONLY,
        };

        bool is_readable() const
        {
            return (mode & READ) == READ;
        };
        bool is_writeable() const
        {
            return (mode & WRITE) == WRITE;
        };

        bool is_32bit() const
        {
            return width == BITS_32;
        }
        bool is_64bit() const
        {
            return width == BITS_64;
        }

        bool is_preserved_on_init() const
        {
            return preserved == PRESERVED_ON_INIT;
        };

        uint64_t read() const
        {
            return rdmsr(addr);
        }
        void write(uint64_t value)
        {
            wrmsr(addr, value);
        }

        /**
     * \brief get address of corresponding mmio register
     *
     * \return mmio register address if this register exists in mmio mode
     */
        std::optional<uint32_t> mmio_addr() const
        {
            if (addr == x86::msr::X2APIC_X2_SELF_IPI) {
                return {};
            }
            static constexpr uint32_t MSR_BASE{ 0x800 };
            static constexpr uint32_t MSR_TO_MMIO_SHIFT{ 4 };
            return lapic_test_tools::LAPIC_START_ADDR + ((addr - MSR_BASE) << MSR_TO_MMIO_SHIFT);
        }

        uint32_t addr;
        mode_t mode;
        width_t width;
        preserve_t preserved;
    };

    using x2apic_msr_table_t = const std::array<x2apic_msr, 44>;
    static x2apic_msr_table_t x2apic_msrs{ {
        { x86::msr::X2APIC_LAPIC_ID, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::NOT_PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LAPIC_VERSION, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TPR, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_PPR, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_EOI, x2apic_msr::WRITE, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LDR, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::NOT_PRESERVED_ON_INIT },
        { x86::msr::X2APIC_SVR, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_31_0, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_63_32, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_95_64, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_127_96, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_159_128, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_191_160, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_223_192, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ISR_255_224, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_31_0, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_63_32, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_95_64, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_127_96, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_159_128, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_191_160, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_223_192, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_TMR_255_224, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_31_0, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_63_32, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_95_64, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_127_96, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_159_128, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_191_160, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_223_192, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_IRR_255_224, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ERR_STS, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_CMCI, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_ICR, x2apic_msr::RW, x2apic_msr::BITS_64, x2apic_msr::NOT_PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_TIMER, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_THERMAL, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_PERF_MON, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_LINT0, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_LINT1, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_LVT_ERR, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_INIT_CNT, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_CURR_CNT, x2apic_msr::READ, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_DIV_CONF, x2apic_msr::RW, x2apic_msr::BITS_32, x2apic_msr::PRESERVED_ON_INIT },
        { x86::msr::X2APIC_X2_SELF_IPI, x2apic_msr::WRITE, x2apic_msr::BITS_32, x2apic_msr::X2APIC_ONLY },
    } };

    /**
 * \brief buffer for MMIO register values
 *
 * Once in x2apic mode, the mmio register space is inaccessible.
 * In order to verify that apic data is preserved when switching modes,
 * the old values are buffered here.
 * It buffers only the values which can be compared, meaning they can
 * be read and are not exclusive to one of the modes.
 */
    class mmio_buffer_t
    {
     public:
        using msr_addr_t = uint32_t;
        using mmio_value_t = uint32_t;

        explicit mmio_buffer_t(x2apic_msr_table_t& msr_table_)
            : msr_table(msr_table_) {}

        mmio_value_t get(const x2apic_msr& msr) const
        {
            auto it{ buffer.find(msr.addr) };
            return it->second;
        }

        void copy_from_mmio()
        {
            for (const auto& msr : msr_table) {
                if (msr.is_readable() and msr.mmio_addr()) {
                    mmio_value_t mmio_val{ lapic_test_tools::read_from_register(msr.mmio_addr().value()) };
                    buffer[msr.addr] = mmio_val;
                }
            }
        }

     private:
        std::map<msr_addr_t, mmio_value_t> buffer;
        x2apic_msr_table_t& msr_table;
    };

    uint64_t read_x2_msr(uint32_t msr);
    void write_x2_msr(uint32_t msr, uint64_t value);

    /**
 * \brief leave x2apic mode
 *
 * In order to leave x2apic mode, the apic is turned off completely,
 * then activated again.
 */
    void x2apic_mode_disable();
    void x2apic_mode_enable();
    bool x2apic_mode_enabled();
    bool x2apic_mode_supported();

    /**
 * \brief switches to x2apic mode on construction and leaves x2apic mode on destruction
 */
    struct x2apic_mode_guard
    {
        /**
     * \param buffer a pointer to an xapic mmio buffer to the current mmio state is copied
     *        before activating x2apic mode
     */
        explicit x2apic_mode_guard(mmio_buffer_t* buffer = nullptr)
        {
            // no interrupt during copying and in between copy and mode switch
            int_guard _;
            if (buffer != nullptr) {
                buffer->copy_from_mmio();
            }
            x2apic_mode_enable();
        }

        x2apic_mode_guard(const x2apic_mode_guard&) = delete;
        x2apic_mode_guard(x2apic_mode_guard&&) = delete;
        x2apic_mode_guard& operator=(const x2apic_mode_guard&) = delete;
        x2apic_mode_guard& operator=(x2apic_mode_guard&&) = delete;

        ~x2apic_mode_guard()
        {
            x2apic_mode_disable();
        }
    };

}  // namespace x2apic_test_tools
