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

volatile uint32_t irq_count{ 0 };
static irqinfo irq_info;

uint8_t const PIC_BASE_VECTOR = 32;
uint8_t const PIC_PIT_IRQ_PIN = 0;
uint8_t const PIC_PIT_IRQ_VECTOR = PIC_BASE_VECTOR + PIC_PIT_IRQ_PIN;
uint8_t const IOAPIC_PIC_IRQ_PIN = 0;
uint8_t const IOAPIC_PIT_TIMER_IRQ_PIN = 2;
uint8_t const IOAPIC_PIT_TIMER_IRQ_VECTOR = 33;
uint8_t const LAPIC_LINT0_PIC_IRQ_VECTOR = 34;

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
    LapicLint0FixedInt
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
 * Drains the interrupts from the PIC.
 *
 * The hardware must be configured in a way, that the PIC can deliver
 * interrupts.
 */
void drain_pic_interrupts(bool assert_pit_interrupt_only)
{
    while (global_pic.get_irr() != 0) {
        irq_info.reset();
        global_pic.unmask_all();
        enable_interrupts_for_single_instruction();
        BARETEST_ASSERT(irq_info.valid);
        // When we are not in the prologue but the actual test, we assert that
        // only IRQs that we expect are pending.
        if (assert_pit_interrupt_only) {
            BARETEST_ASSERT(irq_info.vec == PIC_PIT_IRQ_VECTOR);
        }
        global_pic.eoi();
    }
    global_pic.mask_all();

    // Ensure all interrupts are drained.
    BARETEST_ASSERT(global_pic.get_irr() == 0);
    BARETEST_ASSERT(global_pic.get_isr() == 0);
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
    bool mask_lint0, mask_ioapic_pic, mask_ioapic_pit;

    switch (strategy) {
        case PitInterruptDeliveryStrategy::IoApicPitFixedInt: {
            lapic_enable_safe();
            mask_lint0 = true;
            mask_ioapic_pic = true;
            mask_ioapic_pit = false;
            global_pic.mask(PIC_PIT_IRQ_VECTOR);
            break;
        }
        case PitInterruptDeliveryStrategy::IoApicPicExtInt: {
            lapic_disable_safe();
            mask_lint0 = true;
            mask_ioapic_pic = false;
            mask_ioapic_pit = true;
            global_pic.unmask(PIC_PIT_IRQ_VECTOR);
            break;
        }
        case PitInterruptDeliveryStrategy::LapicLint0FixedInt: {
            lapic_enable_safe();
            mask_lint0 = false;
            mask_ioapic_pic = true;
            mask_ioapic_pit = true;
            global_pic.unmask(PIC_PIT_IRQ_VECTOR);
            break;
        }
    }

    // Better be fail-safe. Only write if the MMIO region logically exists.
    if (lapic_test_tools::global_apic_enabled()) {
        lapic_test_tools::write_lvt_entry(
            lvt_entry::LINT0,
            {
                .vector = LAPIC_LINT0_PIC_IRQ_VECTOR,
                // This looks a bit strange, but the helper that we have is
                // mostly only used for the timer register. This value is
                // ignored by LINT0.
                .timer_mode = lvt_timer_mode ::ONESHOT,
                // TODO we could also deliver this as EXTINT (additionally?)
                .dlv_mode = lvt_dlv_mode::FIXED,
                .mask = mask_lint0 ? lvt_mask::MASKED : lvt_mask::UNMASKED,
            });
    }

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

    if (mask_ioapic_pic) {
        pic_entry.mask();
    }
    else {
        pic_entry.unmask();
    }

    if (mask_ioapic_pit) {
        pit_entry.mask();
    }
    else {
        pit_entry.unmask();
    }

    io_apic.set_irt(pit_entry);
    io_apic.set_irt(pic_entry);
}

static void store_and_count_irq_handler(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    irq_count++;
    info("received interrupt {}", regs->vector);
}

void prologue()
{
    irq_handler::set(store_and_count_irq_handler);

    // In QEMU+TCG, there is sometimes a value in the PIC's IRR when there
    // shouldn't. For safety and maximum platform compatibility, we drain the
    // PIC interrupts.
    global_pit.set_counter(0);
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    drain_pic_interrupts(false);
}

void epilogue()
{
}

void before_test_case_cleanup()
{
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);
    drain_pic_interrupts(true);
    irq_info.reset();
    irq_count = 0;
}

/**
 * Tests that a PIT interrupt can be delivered via the PIC to the guest.
 *
 * The interrupt is either expected by exiting from a HLT or by a busy loop.
 * In the latter case, the (virtualization) platform must deliver an interrupt
 * even if the guest doesn't perform a VM exit on its own. The guest must be
 * forced into an exit, i.e., being poked.
 */
void testcase_receive_pit_interrupt_via_pic(bool busyWaitForInterrupt)
{
    before_test_case_cleanup();
    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPicExtInt);

    BARETEST_ASSERT(global_pic.get_irr() == 0);
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
    BARETEST_ASSERT(global_pic.get_irr() == 0);
    BARETEST_ASSERT(global_pic.get_isr() != 0);

    global_pic.eoi();
    BARETEST_ASSERT(global_pic.get_isr() == 0);

    BARETEST_ASSERT(irq_count == 1);
}

TEST_CASE(receive_pit_interrupt_via_pic_via_hlt)
{
    testcase_receive_pit_interrupt_via_pic(false);
}

// It's sufficient to have the HLT vs busy-loop approach once in this test.
// For the other tests, we just test one variant.
TEST_CASE(receive_pit_interrupt_via_pic_without_vm_exit)
{
    testcase_receive_pit_interrupt_via_pic(true);
}

TEST_CASE(receive_pit_interrupt_via_lapic_via_ioapic)
{
    before_test_case_cleanup();

    prepare_pit_irq_env(PitInterruptDeliveryStrategy::IoApicPitFixedInt);

    global_pit.set_counter(100);
    enable_interrupts_and_halt();
    disable_interrupts();

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == IOAPIC_PIT_TIMER_IRQ_VECTOR);
    BARETEST_ASSERT(not lapic_test_tools::check_irr(IOAPIC_PIT_TIMER_IRQ_VECTOR));

    lapic_test_tools::send_eoi();

    BARETEST_ASSERT(irq_count == 1);
}

// This test currently goes totally crazy in different ways on
// - real hardware (hangs)
// - QEMU+TCG (wrong vector delivered; like IOAPIC started INTA cycle for PIC)
// - QEMU+KVM (HLT is exited but interrupt handler never runs, WTF!)
//   --> I think, the HLT is exited but no interrupt is injected
// - VBox/KVM (hangs)
// - VBox/Vanilla (hangs)
TEST_CASE_CONDITIONAL(receive_pit_interrupt_via_lapic_via_lint0, false /* TODO */)
{
    before_test_case_cleanup();

    prepare_pit_irq_env(PitInterruptDeliveryStrategy::LapicLint0FixedInt);

    global_pit.set_counter(100);
    enable_interrupts_and_halt();
    disable_interrupts();

    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == LAPIC_LINT0_PIC_IRQ_VECTOR);
    BARETEST_ASSERT(not lapic_test_tools::check_irr(LAPIC_LINT0_PIC_IRQ_VECTOR));
    lapic_test_tools::send_eoi();

    BARETEST_ASSERT(irq_count == 1);
}
