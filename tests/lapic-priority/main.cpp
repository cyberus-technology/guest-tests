/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cbl/baretest/baretest.hpp>
#include <toyos/x86/cpuid.hpp>
#include <functional>
#include <toyos/testhelper/hpet.hpp>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/int_guard.hpp>
#include <toyos/testhelper/ioapic.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>
#include <cbl/trace.hpp>
#include <toyos/x86/x86asm.hpp>

using namespace lapic_test_tools;

static constexpr uint8_t PIC_BASE {0x30};
static constexpr uint8_t HPET_IRQ_VEC {PIC_BASE};
static constexpr uint32_t NMI_VECTOR {2};

volatile uint32_t irq_count {0};

std::vector<uint32_t> expected_vectors(fixed_valid_vectors.size());
std::vector<uint32_t> confirmed_vectors;

static ioapic io_apic;
static irqinfo irq_info;
static pic global_pic {PIC_BASE};

static constexpr uint16_t HPET_DESIRED_VENDOR {0x8086};
static constexpr uint8_t HPET_TIMER_NO {0};
static auto hpet_device {hpet::get()};
static auto hpet_timer {hpet_device->get_timer(HPET_TIMER_NO)};

static inline bool hpet_available()
{
    return hpet_device->vendor() == HPET_DESIRED_VENDOR;
}

void prologue()
{
    software_apic_enable();
    write_spurious_vector(SPURIOUS_TEST_VECTOR);
    write_lvt_entry(lvt_entry::LINT0, {.vector = 0, .mode = dlv_mode::EXTINT, .mask = 0});
    irq_handler::guard _(drain_irq);
    enable_interrupts_for_single_instruction();
    std::iota(std::begin(expected_vectors), std::end(expected_vectors), MIN_VECTOR);

    if (hpet_available()) {
        hpet_device->enabled(false);
        hpet_timer->periodic(false);

        hpet_timer->fsb_enabled(false);
        hpet_timer->msi_config(0, 0);
    }
}

void wait_a_sec()
{
    uint32_t divisor = read_from_register(LAPIC_DIVIDE_CONF);
    for (volatile uint32_t i = 0; i < ticks_per_second(divisor); ++i) {}
}

static void lapic_irq_handler(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
}

void wait_until_irq_count_equals(uint32_t num)
{
    while (irq_count < num) {}
}

bool check_if_any_set(uintptr_t reg)
{
    return read_from_register(reg) == 0;
}

void poll_pic_irr()
{
    const uint16_t mask = 0b1;
    while ((global_pic.get_irr() & mask) == 0) {}
}

bool is_isr_and_irr_empty()
{
    bool empty = true;
    for (uintptr_t isr = ISR_32_63; isr <= ISR_224_255; isr += LAPIC_REG_STRIDE) {
        empty &= check_if_any_set(isr);
    }
    for (uintptr_t irr = IRR_32_63; irr <= IRR_224_255; irr += LAPIC_REG_STRIDE) {
        empty &= check_if_any_set(irr);
    }
    return empty;
}

void drain_interrupts()
{
    irq_handler::guard _(lapic_irq_handler);
    lapic_set_task_priority(0x0);
    while (not is_isr_and_irr_empty()) {
        enable_interrupts_for_single_instruction();
        send_eoi();
    }
}

void wait_for_interrupts(uint32_t expected_irq_count)
{
    while (confirmed_vectors.size() != expected_irq_count) {
        enable_interrupts_and_halt();
        disable_interrupts();
        if (irq_info.valid) {
            if (fixed_valid_vectors.contains(static_cast<uint32_t>(irq_info.vec))) {
                confirmed_vectors.push_back(static_cast<uint32_t>(irq_info.vec));
                irq_info.reset();
                send_eoi();
            } else {
                irq_info.reset();
            }
        }
    }
}

static inline bool hv_bit_present()
{
    return cpuid(CPUID_LEAF_FAMILY_FEATURES).ecx & LVL_0000_0001_ECX_HV;
}

void test_lapic_priority(dest_sh sh)
{
    drain_interrupts();
    confirmed_vectors.clear();

    irq_info.reset();
    irq_handler::guard _(lapic_irq_handler);

    for (uint32_t vector : expected_vectors) {
        send_self_ipi(vector, sh);
    }

    wait_for_interrupts(expected_vectors.size());

    std::reverse(confirmed_vectors.begin(), confirmed_vectors.end());

    BARETEST_ASSERT((expected_vectors == confirmed_vectors));
}

TEST_CASE(lapic_priority_ipi_shorthand)
{
    test_lapic_priority(dest_sh::SELF);
}

TEST_CASE(lapic_priority_ipi_no_shorthand)
{
    test_lapic_priority(dest_sh::NO_SH);
}

void test_interrupt_injection_should_honor_tpr_value(dest_sh sh)
{
    drain_interrupts();

    // here we only test the values from 0xe and 0x2 because:
    // value 0xf should inhibit all interrupts and it was easier to simply write another testcase for this
    // values 0x1 and 0x0 are mapped to exceptions which are not tested here

    irq_info.reset();
    irq_handler::guard _(lapic_irq_handler);

    for (uint32_t priority = 0xe; priority >= 0x2; --priority) {
        confirmed_vectors.clear();
        // for each priority, we allow only 16 (VECTORS_PER_CLASS) interrupts.
        const uint32_t num_allowed_vectors = (MAX_VECTOR + 1) - ((priority + 1) * VECTORS_PER_CLASS);
        for (uint32_t vector : expected_vectors) {
            send_self_ipi(vector, sh);
        }

        lapic_set_task_priority(priority);
        wait_for_interrupts(num_allowed_vectors);

        for (uint32_t& vec : confirmed_vectors) {
            BARETEST_ASSERT((vec > MAX_VECTOR - num_allowed_vectors));
        }

        BARETEST_ASSERT((confirmed_vectors.size() == num_allowed_vectors));

        lapic_set_task_priority(0x0);
        wait_for_interrupts(expected_vectors.size());
    }
}

TEST_CASE(interrupt_injection_should_honor_tpr_value_ipi_shorthand)
{
    test_interrupt_injection_should_honor_tpr_value(dest_sh::SELF);
}

TEST_CASE(interrupt_injection_should_honor_tpr_value_ipi_no_shorthand)
{
    test_interrupt_injection_should_honor_tpr_value(dest_sh::NO_SH);
}

static void lapic_irq_handler_tpr(intr_regs* regs)
{
    // i need this condition, otherwise a spurious interrupt would be able to make the condition in the next test-case
    // become true, and the assertion fails
    if (fixed_valid_vectors.contains(regs->vector)) {
        irq_info.record(regs->vector, regs->error_code);
    }
}

void test_setting_lapic_tpr_to_f_should_inhibit_all_interrupts(dest_sh sh)
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler_tpr);
    irq_info.reset();
    confirmed_vectors.clear();
    // the value for the self ipi is just a random vector
    send_self_ipi(MAX_VECTOR, sh);

    lapic_set_task_priority(0xf);
    enable_interrupts_for_single_instruction();
    BARETEST_ASSERT(not irq_info.valid);
    lapic_set_task_priority(0x0);

    // in this case there shouldnt be more than one interrupt
    wait_for_interrupts(1);
    BARETEST_ASSERT((confirmed_vectors.size() == 1));
    BARETEST_ASSERT((confirmed_vectors.front() == MAX_VECTOR));
}

TEST_CASE(setting_lapic_tpr_to_f_should_inhibit_all_interrupts_ipi_shorthand)
{
    test_setting_lapic_tpr_to_f_should_inhibit_all_interrupts(dest_sh::SELF);
}

TEST_CASE(setting_lapic_tpr_to_f_should_inhibit_all_interrupts_ipi_no_shorthand)
{
    test_setting_lapic_tpr_to_f_should_inhibit_all_interrupts(dest_sh::NO_SH);
}

void drain_inhibited_interrupts()
{
    irq_handler::guard _(lapic_irq_handler);
    lapic_set_task_priority(0x0);
    enable_interrupts_for_single_instruction();
    while (irq_info.valid) {
        send_eoi();
        irq_info.reset();
        enable_interrupts_for_single_instruction();
    }
}

void check_ppr_register(uint32_t isrv)
{
    uint32_t tpr_data = read_from_register(LAPIC_TPR);
    uint32_t ppr_data = read_from_register(LAPIC_PPR);

    uint32_t tpr_7_4 = (tpr_data >> 4) & 0xf;
    uint32_t isr_7_4 = (isrv >> 4) & 0xf;
    uint32_t ppr_7_4 = (ppr_data >> 4) & 0xf;

    uint32_t ppr_7_4_expected = std::max(tpr_7_4, isr_7_4);

    BARETEST_ASSERT((ppr_7_4 == ppr_7_4_expected));

    uint32_t tpr_3_0 = tpr_data & 0xf;
    uint32_t ppr_3_0 = ppr_data & 0xf;

    if (tpr_7_4 > isr_7_4) {
        BARETEST_ASSERT((ppr_3_0 == tpr_3_0));
    } else if (tpr_7_4 < isr_7_4) {
        BARETEST_ASSERT((ppr_3_0 == 0));
    } else {
        BARETEST_ASSERT(((ppr_3_0 == tpr_3_0) or (ppr_3_0 == 0)));
    }
}

static void lapic_irq_handler_ppr(intr_regs* regs)
{
    check_ppr_register(regs->vector);
    ++irq_count;
    send_eoi();
}

void test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh sh)
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler_ppr);
    for (uint32_t priority = 0xe; priority >= 0x2; --priority) {
        const uint32_t num_allowed_vectors = (MAX_VECTOR + 1) - ((priority + 1) * VECTORS_PER_CLASS);
        for (uint32_t vector : expected_vectors) {
            send_self_ipi(vector, sh);
        }

        lapic_set_task_priority(priority);
        irq_count = 0;

        enable_interrupts();
        wait_until_irq_count_equals(num_allowed_vectors);
        disable_interrupts();

        drain_interrupts();
    }
}

TEST_CASE(ppr_value_for_all_combinations_of_tpr_and_isrv_ipi_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::SELF);
}

TEST_CASE(ppr_value_for_all_combinations_of_tpr_and_isrv_no_ipi_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::NO_SH);
}

void test_ppr_value_for_highest_priority(dest_sh sh)
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler);
    irq_info.reset();

    lapic_set_task_priority(0xf);
    send_self_ipi(MAX_VECTOR, sh);

    // we inhibited all interrupts, so i have to check with zero
    check_ppr_register(0);
    enable_interrupts_for_single_instruction();

    BARETEST_ASSERT(not(irq_info.valid));
}

TEST_CASE(ppr_value_for_highest_priority_ipi_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::SELF);
}

TEST_CASE(ppr_value_for_highest_priority_ipi_no_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::NO_SH);
}

static void lapic_irq_handler_ppr2(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    lapic_set_task_priority(0xe);
    check_ppr_register(regs->vector);
    lapic_set_task_priority(0x0);
    send_eoi();
}

void test_ppr_changing_tpr_value_inside_the_irq_handler_should_work(dest_sh sh)
{
    drain_interrupts();

    // one interrupt, where bits 7-4 are higher than the priority that we set inside the handler, one
    // where bits 7-4 are equal to the priority, and one where bits 7-4 are lower

    irq_handler::guard _(lapic_irq_handler_ppr2);
    lapic_set_task_priority(0x0);
    disable_interrupts();

    irq_info.reset();
    send_self_ipi(0xf0, sh);
    enable_interrupts_for_single_instruction();
    BARETEST_ASSERT(irq_info.valid);

    irq_info.reset();
    send_self_ipi(0xe0, sh);
    enable_interrupts_for_single_instruction();
    BARETEST_ASSERT(irq_info.valid);

    irq_info.reset();
    send_self_ipi(0xd0, sh);
    enable_interrupts_for_single_instruction();
    BARETEST_ASSERT(irq_info.valid);
}

TEST_CASE(ppr_changing_tpr_value_inside_the_irq_handler_should_work_ipi_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::SELF);
}

TEST_CASE(ppr_changing_tpr_value_inside_the_irq_handler_should_work_ipi_no_shorthand)
{
    test_ppr_value_for_all_combinations_of_tpr_and_isrv(dest_sh::NO_SH);
}

/* How does the "self_nmi_should_call..."-test work:
 * At first I send a self-ipi, which shouldnt occur due to the disabled interrupts.
 * Then I send a nmi, which should ignore the disabled interrupts.
 * We enter the irq handler the first time, irq_count will be incremented to 1 and we are in case one:
 *   I send another nmi, which shouldnt occur due to the block nmi status, and we enable the interrupts.
 * Now the ipi occurs, we increment irq_count to 2 and enter case two:
 *   We just check if the irq that called the handler isnt a nmi.
 *   We leave the irq_handler, iret is called and the second nmi occurs
 * We increment icq_count to 3, and case three checks if the calling irq is the nmi.
 */
static void lapic_irq_handler_nmi(intr_regs* regs)
{
    ++irq_count;
    if (irq_count == 1) {
        send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);
        enable_interrupts_for_single_instruction();
    } else if (irq_count == 2) {
        BARETEST_ASSERT((regs->vector != NMI_VECTOR));
    } else if (irq_count == 3) {
        BARETEST_ASSERT((regs->vector == NMI_VECTOR));
    }
}

TEST_CASE(self_nmi_should_call_handler_while_interrupts_are_closed_and_second_nmi_should_happen_after_ipi)
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler_nmi);
    irq_count = 0;
    send_self_ipi(MAX_VECTOR);
    send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);

    wait_until_irq_count_equals(3);
    // the eoi for the ipi above
    send_eoi();
}

static void lapic_irq_handler_two_nmis(intr_regs*)
{
    if (irq_count == 0) {
        send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);
    }
    ++irq_count;
}

TEST_CASE(sending_an_nmi_while_handling_another_should_work)
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler_two_nmis);
    irq_count = 0;
    send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);

    wait_until_irq_count_equals(2);
}

// This test is broken on VirtualBox, so we skip it if we find the HV bit
TEST_CASE_CONDITIONAL(sending_self_nmi_with_shorthand_shouldnt_work, not hv_bit_present())
{
    drain_interrupts();

    irq_handler::guard _(lapic_irq_handler);
    irq_info.reset();

    send_self_ipi(NMI_VECTOR, dest_sh::SELF, dest_mode::PHYSICAL, dlv_mode::NMI);

    wait_a_sec();
    BARETEST_ASSERT(not(irq_info.valid));
}

using trg_mode = hpet::timer::trigger;
struct hpet_test_config
{
    static hpet_test_config legacy() { return {.legacy_active = true, .ioapic_gsi = 0, .trigger = trg_mode::EDGE}; }

    static hpet_test_config ioapic(uint8_t gsi, trg_mode trigger_mode)
    {
        return {.legacy_active = false, .ioapic_gsi = gsi, .trigger = trigger_mode};
    }

    bool legacy_active;
    uint8_t ioapic_gsi;
    trg_mode trigger;
};

struct hpet_test_ctx
{
    hpet_test_ctx(const hpet_test_config& cfg_) : cfg(cfg_)
    {
        hpet_timer->int_enabled(false);
        hpet_timer->periodic(false);
        hpet_device->legacy_enabled(cfg.legacy_active);
        hpet_timer->ioapic_gsi(cfg.ioapic_gsi);
        hpet_timer->trigger_mode(cfg.trigger);

        if (cfg.legacy_active) {
            global_pic.unmask(HPET_IRQ_VEC);
        }
    }

    ~hpet_test_ctx()
    {
        hpet_timer->int_enabled(false);
        hpet_timer->periodic(false);
        hpet_device->enabled(false);

        if (cfg.legacy_active) {
            global_pic.mask(HPET_IRQ_VEC);
            global_pic.eoi(HPET_IRQ_VEC);
        }
    }

    void arm(bool periodic = false)
    {
        hpet_timer->int_enabled(true);
        hpet_device->main_counter(0);

        /* It is important to initialize the hpet timer in the correct order,
         * otherwise the last test will fail at least in VirtualBox. It looks
         * like VirtualBox drops the write to the comparator if the timer isn't
         * configured for periodic interrupts.
         */
        hpet_timer->periodic(periodic);
        hpet_timer->comparator(target);

        hpet_device->enabled(true);
    }

    const uint64_t target {hpet_device->milliseconds_to_ticks(100)};
    const hpet_test_config cfg;
};

static void receive_extint_while_handling_a_fixed_interrupt_handler(intr_regs* regs)
{
    BARETEST_ASSERT(regs->vector == HPET_IRQ_VEC);
    irq_count++;
}

static void send_extint_while_handling_a_fixed_interrupt_handler(intr_regs* regs)
{
    BARETEST_ASSERT(regs->vector == MAX_VECTOR);
    irq_count++;

    irq_handler::guard _(receive_extint_while_handling_a_fixed_interrupt_handler);

    hpet_test_ctx hpet_ctx(hpet_test_config::legacy());
    hpet_ctx.arm();
    poll_pic_irr();

    int_guard interrupt_guard(int_guard::irq_status::enabled_and_halted);
}

/* This test does the following:
 * At first, i send a self-ipi and enable interrupts. I should enter the first irq
 * handler (send_extint_...), and inside the handler, I program the hpet to send
 * another interrupt. I unmask the hpet at the pic and enable interrupts again,
 * should get the interrupt and enter the second interrupt handler.
 */
void test_receiving_an_extint_while_handling_a_fixed_interrupt_should_be_possible(dest_sh sh)
{
    drain_interrupts();

    irq_handler::guard _(send_extint_while_handling_a_fixed_interrupt_handler);
    irq_count = 0;

    send_self_ipi(MAX_VECTOR, sh);
    int_guard interrupt_guard(int_guard::irq_status::enabled_and_halted);

    BARETEST_ASSERT(irq_count == 2);
}

TEST_CASE_CONDITIONAL(receiving_an_extint_while_handling_a_fixed_interrupt_should_be_possible_ipi_shorthand,
                      hpet_available())
{
    test_receiving_an_extint_while_handling_a_fixed_interrupt_should_be_possible(dest_sh::SELF);
}

TEST_CASE_CONDITIONAL(receiving_an_extint_while_handling_a_fixed_interrupt_should_be_possible_ipi_no_shorthand,
                      hpet_available())
{
    test_receiving_an_extint_while_handling_a_fixed_interrupt_should_be_possible(dest_sh::NO_SH);
}

static void receive_fixed_interrupt_while_handling_an_extint_handler(intr_regs* regs)
{
    BARETEST_ASSERT(regs->vector == MAX_VECTOR);
    irq_count++;
}

static void send_fixed_interrupt_while_handling_an_extint_handler(intr_regs* regs)
{
    BARETEST_ASSERT(regs->vector == HPET_IRQ_VEC);
    irq_count++;

    irq_handler::guard _(receive_fixed_interrupt_while_handling_an_extint_handler);
    send_self_ipi(MAX_VECTOR);
    int_guard interrupt_guard(int_guard::irq_status::enabled_and_halted);
}

/* This test does the following:
 * At first, I program the hpet to send me an interrupt and unmask it at the pic.
 * Then I enable the interrupts and find myself in the first interrupt handler.
 * I send an ipi, enable the interrupts and I am now in the second interrupt handler.
 * I check the interrupt vector everytime to see if the right interrupt brought me
 * there.
 */
TEST_CASE_CONDITIONAL(receiving_a_fixed_interrupt_while_handling_an_extint_should_be_possible, hpet_available())
{
    drain_interrupts();

    irq_handler::guard _(send_fixed_interrupt_while_handling_an_extint_handler);
    irq_count = 0;

    hpet_test_ctx hpet_ctx(hpet_test_config::legacy());
    hpet_ctx.arm();
    poll_pic_irr();

    int_guard interrupt_guard(int_guard::irq_status::enabled_and_halted);

    BARETEST_ASSERT(irq_count == 2);
}

static bool got_ipi {false};
static bool got_extint {false};
static bool got_nmi {false};

static void handle_multiple(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    if (regs->vector == MAX_VECTOR) {
        got_ipi = true;
    } else if (regs->vector == HPET_IRQ_VEC) {
        got_extint = true;
    } else if (regs->vector == NMI_VECTOR) {
        got_nmi = true;
    }
}

// This test is broken on VirtualBox, so we skip it if we find the HV bit
TEST_CASE_CONDITIONAL(receiving_extint_and_fixed_interrupt_simultaneously_should_deliver_both,
                      not hv_bit_present() and hpet_available())
{
    drain_interrupts();
    disable_interrupts();

    irq_handler::guard _(handle_multiple);

    // prepare extint
    hpet_test_ctx hpet_ctx(hpet_test_config::legacy());
    hpet_ctx.arm();

    // prepare fixed int
    send_self_ipi(MAX_VECTOR);

    wait_a_sec();
    poll_pic_irr();
    enable_interrupts_for_single_instruction();

    BARETEST_ASSERT(got_ipi);
    BARETEST_ASSERT(got_extint);

    wait_a_sec();

    got_ipi = false;
    got_extint = false;
}

TEST_CASE_CONDITIONAL(receiving_nmi_and_fixed_interrupt_simultaneously_should_deliver_both, hpet_available())
{
    drain_interrupts();

    disable_interrupts();

    irq_handler::guard _(handle_multiple);
    // prepare exception

    // prepare fixed int
    send_self_ipi(MAX_VECTOR);
    send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);

    wait_a_sec();
    enable_interrupts_for_single_instruction();

    BARETEST_ASSERT(got_ipi);
    BARETEST_ASSERT(got_nmi);

    got_ipi = false;
    got_nmi = false;
}

static constexpr size_t NUM_FAST_NMI {10000};
static uint8_t hpet_gsi {0};

static void fast_nmi_test_handler(intr_regs* regs)
{
    BARETEST_ASSERT(regs->vector == 2);
    ++irq_count;

    if (irq_count < NUM_FAST_NMI) {
        // We're in NMI_BLOCKING state, send another NMI so a VMM has to request an NMI window at this point.
        send_self_ipi(NMI_VECTOR, dest_sh::NO_SH, dest_mode::PHYSICAL, dlv_mode::NMI);
    } else {
        // Disable periodic NMI by masking the redirection entry
        auto irt {io_apic.get_irt(hpet_gsi)};
        irt.mask();
        io_apic.set_irt(irt);
    }
}

// This test checks whether the VMM handles concurrent NMI requests correctly. To do so, we
// send an NMI to the VCPU and request another NMI while in NMI_BLOCKING state. This causes the
// NMI handler to be executed all the time. At the same time, we program the redirection entry
// for the HPET to also generate an NMI. The hope here is that the NMI triggered by the HPET will
// arrive while the VMM is handling the other NMI.
TEST_CASE_CONDITIONAL(fast_triggering_NMIs_should_not_kill_vmm, hpet_available())
{
    drain_interrupts();
    irq_count = 0;

    irq_handler::guard _(fast_nmi_test_handler);

    hpet_gsi = math::order_max(hpet_timer->available_gsis());
    hpet_test_ctx hpet_ctx(hpet_test_config::ioapic(hpet_gsi, trg_mode::EDGE));
    hpet_ctx.arm(true);

    using redirection_entry = ioapic::redirection_entry;
    auto lapic_id {(read_from_register(LAPIC_ID) >> LAPIC_ID_SHIFT) & LAPIC_ID_MASK};
    auto irt {redirection_entry(hpet_gsi, NMI_VECTOR, lapic_id, redirection_entry::trigger::EDGE)};
    irt.delivery_mode(ioapic::redirection_entry::dlv_mode::NMI);
    io_apic.set_irt(irt);

    while (irq_count < NUM_FAST_NMI) {};

    irt.mask();
    io_apic.set_irt(irt);
}

BARETEST_RUN;
