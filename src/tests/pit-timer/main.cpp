/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

/**
 * This tests various delivery strategies of the PIT timer interrupt on modern
 * APIC-enabled x86 platforms. This test transitively also tests functionality
 * of the PIC, the LAPIC, and the IOAPIC.
 *
 * In hardware, the connection between the components looks roughly as shown
 * in the following:
 *
 *         [BSP]           [CPU]
 *     INTR | | NMI    INTR | | NMI
 *          | |             | |
 *  LINT0-[LAPIC]         [LAPIC]
 *   |       |               |
 *   |      ============================= APIC Bus
 *   |              |            |
 *   |          [IOAPIC 0]    ([IOAPIC N])
 *   |           ||||||||       ||||||||
 *   ------|-----| |
 *    INTR |       |
 *         |       ----
 *      [PIC Master]  |
 *        |||         |
 *        |           |
 *        ------------| INTR
 *               [PIT]
 *
 * The PIT's interrupt output pin is hard-wired to PIN 0 of the PIC and PIN 2 of
 * the IOAPIC. The INTR output of the PIC is hard-wired to PIN 0 of the IOAPIC
 * and to LINT0 of the LAPIC. The IOAPIC redirection table entry can be
 * configured as ExtInt so that the CPU will perform an INTA cycle to retrieve
 * the interrupt vector from the PIC.
 *
 * By (un)masking the corresponding interrupts, an operating system can decide
 * in which way it wants to receive timer interrupts from the PIT.
 */

#include "toyos/testhelper/lapic_lvt_guard.hpp"
#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/ioapic.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>
#include <toyos/testhelper/pit.hpp>

using redirection_entry = ioapic::redirection_entry;
using lvt_entry = lapic_test_tools::lvt_entry;
using lvt_mask = lapic_test_tools::lvt_mask;
using lvt_dlv_mode = lapic_test_tools::lvt_dlv_mode;

volatile uint32_t irq_count = 0;
static irqinfo irq_info;

uint8_t const PIC_BASE_VECTOR = 32;
uint8_t const PIC_PIT_IRQ_PIN = 0;

/// Effective vector of PIT interrupt when delivered from PIC as ExtInt.
uint8_t const PIC_PIT_IRQ_VECTOR = PIC_BASE_VECTOR + PIC_PIT_IRQ_PIN;
uint8_t const IOAPIC_PIC_IRQ_PIN = 0;
uint8_t const IOAPIC_PIT_TIMER_IRQ_PIN = 2;
/// Effective vector of PIT interrupt when delivered via IOAPIC as fixed.
uint8_t const IOAPIC_PIT_TIMER_IRQ_VECTOR = PIC_BASE_VECTOR + pic::PINS + 1;
/// Effective vector of PIT interrupt when delivered via lint0 as fixed.
uint8_t const LAPIC_LINT0_PIC_IRQ_VECTOR = IOAPIC_PIT_TIMER_IRQ_VECTOR + 1;

static pic global_pic{ PIC_BASE_VECTOR };
static pit global_pit{ pit::operating_mode::INTERRUPT_ON_TERMINAL_COUNT };

BARETEST_RUN;

/**
 * A subset of possible PIT interrupt delivery strategies.
 */
enum class PitInterruptDeliveryStrategy
{
    /**
     * The PIT interrupt is delivered as FIXED interrupt (IOAPIC pin 2).
     */
    IoApicPitFixedInt,
    /**
     * The PIT interrupt is raised in the PIC and the PIC raises an interrupt
     * in the IOAPIC that is configured as ExtInt interrupt (IOAPIC pin 0).
     */
    IoApicPicExtInt,
    /**
     * The PIT interrupt is raised in the PIC and the PIC raises the interrupt
     * in the LINT0 register of the LAPIC as FIXED interrupt.
     */
    LapicLint0FixedInt,
};

/**
 * Safely disables the LAPIC, both in software and "hardware". Experiments
 * showed that virtualization solutions such as VBox and KVM are more
 * conservative than actual hardware. Specifically, if a guest expects to get
 * a PIT interrupt delivered via the PIC via an ExtInt-configured redirection
 * entry in the IOAPIC, KVM and VBOX will get stuck as they expect the interrupt
 * to arrive in LAPICs LINT0 line, unless the LAPIC is disabled in hardware.
 */
void lapic_disable_safe()
{
    if (lapic_test_tools::software_apic_enabled()) {
        lapic_test_tools::software_apic_disable();
    }
    if (lapic_test_tools::global_apic_enabled()) {
        lapic_test_tools::global_apic_disable();
    }
}

/**
 * Counterpart for `lapic_disable_safe()`.
 */
void lapic_enable_safe()
{
    if (not lapic_test_tools::global_apic_enabled()) {
        lapic_test_tools::global_apic_enable();
    }
    if (not lapic_test_tools::software_apic_enabled()) {
        lapic_test_tools::software_apic_enable();
    }
}

/**
 * Drains the PIT interrupt from the PIC. We don't drain other interrupts as
 * we don't use them or rely on them in the test. Experience showed that some
 * machines raise for example the serial interrupt (pin 3) which breaks the
 * irr==0 assertion we used to have after EOI'ing the PIT.
 *
 * The hardware must be configured in a way, that the PIC can deliver
 * interrupts.
 */
void drain_pic_pit_interrupt()
{
    if (global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR)) {
        global_pic.unmask(PIC_PIT_IRQ_VECTOR);
        enable_interrupts_for_single_instruction();
        BARETEST_ASSERT(irq_info.valid);
        BARETEST_ASSERT(irq_info.vec == PIC_PIT_IRQ_VECTOR);
        BARETEST_ASSERT(global_pic.highest_pending_isr_vec() == std::make_optional(PIC_PIT_IRQ_VECTOR));
        global_pic.mask(PIC_PIT_IRQ_VECTOR);
        global_pic.eoi();
    }

    // Ensure the PIT interrupt didn't fire again.
    BARETEST_ASSERT(not global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR));
    BARETEST_ASSERT(global_pic.get_isr() == 0);
}

/**
 * Helper for `prepare_pit_irq_env` that configures LAPIC#lint0.
 */
void configure_lapic(bool unmask_lint0, lvt_dlv_mode dlv_mode = lvt_dlv_mode::FIXED)
{
    uint32_t vector;

    switch (dlv_mode) {
        case lvt_dlv_mode::FIXED: {
            vector = LAPIC_LINT0_PIC_IRQ_VECTOR;
            break;
        }
        case lvt_dlv_mode::EXTINT: {
            vector = 0 /* ignored */;
            break;
        }
        default:
            baretest::fail("Invalid dlv_mode %d\n", dlv_mode);
    }

    // The interrupt configuration that works on all our machines.
    auto pin_polarity = lvt_pin_polarity::ACTIVE_HIGH;
    auto trigger_mode = lvt_trigger_mode::EDGE;

    lapic_test_tools::write_lvt_entry(
        lvt_entry::LINT0,
        lvt_entry_t::lintX(
            vector,
            unmask_lint0 ? lvt_mask::UNMASKED : lvt_mask::MASKED,
            pin_polarity,
            trigger_mode,
            dlv_mode));
}

/**
 * Helper for `prepare_pit_irq_env` that configures the IOAPIC's redirection
 * entries.
 */
void configure_ioapic(bool unmask_pic_pin, bool unmask_pit_pin)
{
    ioapic io_apic;
    redirection_entry pit_entry(IOAPIC_PIT_TIMER_IRQ_PIN,
                                IOAPIC_PIT_TIMER_IRQ_VECTOR,
                                0 /* BSP: physical destination mode */,
                                redirection_entry::dlv_mode::FIXED,
                                redirection_entry::trigger_mode::EDGE);
    redirection_entry pic_entry(IOAPIC_PIC_IRQ_PIN,
                                0 /* EXTINT; no vector raised by IOAPIC */,
                                0 /* BSP: physical destination mode */,
                                redirection_entry::dlv_mode::EXTINT,
                                redirection_entry::trigger_mode::EDGE);

    if (unmask_pic_pin) {
        pic_entry.unmask();
    }
    else {
        pic_entry.mask();
    }

    if (unmask_pit_pin) {
        pit_entry.unmask();
    }
    else {
        pit_entry.mask();
    }

    io_apic.set_irt(pit_entry);
    io_apic.set_irt(pic_entry);
}

/**
 * Sets the correct interrupt configurations at all destinations where the PIT
 * timer interrupt is effectively raised. This function programs the LAPIC
 * (LINT0), the PIC, and the IOAPIC (redirection entries) correspondingly.
 *
 * After that, the PIT interrupt will raise the correct and expected vector
 * via the desired delivery strategy.
 */
void prepare_pit_irq_env(PitInterruptDeliveryStrategy strategy)
{
    switch (strategy) {
        case PitInterruptDeliveryStrategy::IoApicPitFixedInt: {
            // LAPIC must be enabled as it receives and forwards the FIXED
            // interrupt.
            lapic_enable_safe();
            global_pic.mask(PIC_PIT_IRQ_VECTOR);
            configure_ioapic(false, true);
            configure_lapic(false);
            break;
        }
        case PitInterruptDeliveryStrategy::IoApicPicExtInt: {
            // Some (non-spec compliant) platforms such as VirtualBox expect
            // ExtInt interrupt via LINT0 instead of the IOAPIC. Hence, we
            // disable the LAPIC for maximum safety in this test case.
            lapic_disable_safe();
            global_pic.unmask(PIC_PIT_IRQ_VECTOR);
            configure_ioapic(true, false);
            break;
        }
        case PitInterruptDeliveryStrategy::LapicLint0FixedInt: {
            lapic_enable_safe();
            global_pic.unmask(PIC_PIT_IRQ_VECTOR);
            configure_ioapic(false, false);
            configure_lapic(true, lvt_dlv_mode::FIXED);
            break;
        }
    }
}

static void store_and_count_irq_handler(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    irq_count++;

    // Sanity checks to simplify debugging if something goes wrong.
    switch (regs->vector) {
        case LAPIC_LINT0_PIC_IRQ_VECTOR:
        case PIC_PIT_IRQ_VECTOR:
        case IOAPIC_PIT_TIMER_IRQ_VECTOR: {
            break;
        }
        case PIC_BASE_VECTOR + pic::SPURIOUS_IRQ: {
            baretest::fail("Unexpected vector! Got PIC's spurious vector.\n");
            break;
        }
        default: {
            baretest::fail("Unexpected vector! Got %d\n", regs->vector);
            break;
        }
    }
}

void prologue()
{
    irq_handler::set(store_and_count_irq_handler);

    // Reset/disable counter for maximum safety.
    global_pit.set_counter(0);
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    drain_pic_pit_interrupt();
}

void epilogue()
{
}

void before_test_case_cleanup()
{
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    drain_pic_pit_interrupt();
    irq_info.reset();
    irq_count = 0;
}

/**
 * Tests that a PIT interrupt can be delivered via the PIC to the guest. This
 * only works if configured as ExtInt interrupt, so that the hardware performs
 * an INTA cycle.
 *
 * The interrupt is either expected by exiting from a HLT or by a busy loop.
 * In the latter case, the (virtualization) platform must deliver an interrupt
 * even if the guest doesn't perform a VM exit on its own. The guest must be
 * forced into an exit, i.e., being poked.
 */
void receive_pit_interrupt_via_pic(bool busyWaitForInterrupt)
{
    before_test_case_cleanup();
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);

    BARETEST_ASSERT(not global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR));
    BARETEST_ASSERT(global_pic.get_isr() == 0);

    global_pit.set_counter(100);

    if (busyWaitForInterrupt) {
        enable_interrupts();
        while (not irq_info.valid) {
        }
    }
    else {
        enable_interrupts_and_halt();
    }
    disable_interrupts();

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == PIC_PIT_IRQ_VECTOR);
    BARETEST_ASSERT(global_pic.highest_pending_isr_vec() == std::make_optional(PIC_PIT_IRQ_VECTOR));

    global_pic.eoi();
    BARETEST_ASSERT(not global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR));
    BARETEST_ASSERT(global_pic.get_isr() == 0);

    BARETEST_ASSERT(irq_count == 1);
}

// It's sufficient to have the HLT vs busy-loop approach only once or a few
// times in this test but not for every test case.
TEST_CASE(pit_irq_via_ioapic_pic_extint__hlt)
{
    before_test_case_cleanup();
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    receive_pit_interrupt_via_pic(false);
}

TEST_CASE(pit_irq_via_ioapic_pic_extint__without_vm_exit)
{
    before_test_case_cleanup();
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    receive_pit_interrupt_via_pic(true);
}

TEST_CASE(pit_irq_via_ioapic_fixed)
{
    before_test_case_cleanup();

    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPitFixedInt);

    BARETEST_ASSERT(not global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR));
    BARETEST_ASSERT(global_pic.get_isr() == 0);

    global_pit.set_counter(100);
    enable_interrupts_and_halt();
    disable_interrupts();

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == IOAPIC_PIT_TIMER_IRQ_VECTOR);
    BARETEST_ASSERT(not lapic_test_tools::check_irr(IOAPIC_PIT_TIMER_IRQ_VECTOR));

    lapic_test_tools::send_eoi();

    BARETEST_ASSERT(irq_count == 1);
}

TEST_CASE(pit_irq_via_lapic_lint0_fixed)
{
    before_test_case_cleanup();

    prepare_pit_irq_env(PitInterruptDeliveryStrategy::LapicLint0FixedInt);

    BARETEST_ASSERT(not global_pic.vector_in_irr(PIC_PIT_IRQ_VECTOR));
    BARETEST_ASSERT(global_pic.get_isr() == 0);

    global_pit.set_counter(100);
    enable_interrupts_and_halt();
    disable_interrupts();

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == LAPIC_LINT0_PIC_IRQ_VECTOR);
    BARETEST_ASSERT(not lapic_test_tools::check_irr(LAPIC_LINT0_PIC_IRQ_VECTOR));
    lapic_test_tools::send_eoi();

    BARETEST_ASSERT(irq_count == 1);
}
