/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

/**
 * LAPIC timer tests. Tests the three timer modes
 * - oneshot,
 * - periodic, and
 * - TSC deadline.
 *
 * The latter is only tested if it is supported by the platform.
 */

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_lvt_guard.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/x86/x86defs.hpp>

using namespace lapic_test_tools;

using x86::msr;

volatile uint32_t irq_count{ 0 };
volatile uint64_t start_time{ 0 };
volatile uint64_t finish_time{ 0 };
// Only properly initialized when hypervisor supports TSC deadline mode.
volatile uint32_t tsc_to_bus_ratio{ 0 };
static constexpr uint32_t EXPECTED_IRQS{ 128 };
static constexpr uint32_t TIMER_INIT_COUNT{ 4096 };
static constexpr uint32_t DEADLINE_OFFSET{ 131072 };

static irqinfo irq_info;

void calibrating_irq_handler(intr_regs* regs)
{
    finish_time = rdtsc();
    irq_info.record(regs->vector, regs->error_code);
    send_eoi();
}

void prologue()
{
    mask_pic();
    software_apic_enable();
    write_spurious_vector(SPURIOUS_TEST_VECTOR);
    {
        irq_handler::guard handler_guard(drain_irq);
        enable_interrupts_for_single_instruction();
    }

    // Determine the tsc-to-bus ratio
    if (!supports_tsc_deadline_mode()) {
        printf("WARN: TSC_DEADLINE mode not supported\n");
    }
    else {
        irq_info.reset();
        irq_handler::guard handler_guard(calibrating_irq_handler);
        write_divide_conf(1);
        lvt_guard lvt_guard(lvt_entry::TIMER, MAX_VECTOR, lvt_timer_mode::ONESHOT);
        enable_interrupts();
        write_to_register(LAPIC_INIT_COUNT, DEADLINE_OFFSET);
        start_time = rdtsc();

        while (not irq_info.valid) {
        };

        uint64_t oneshot_time = finish_time - start_time;

        irq_info.reset();
        write_lvt_timer_mode(lvt_entry::TIMER, lvt_timer_mode::DEADLINE);
        wrmsr(msr::IA32_TSC_DEADLINE, rdtsc() + DEADLINE_OFFSET);
        start_time = rdtsc();

        while (not irq_info.valid) {
        };

        uint64_t deadline_time = finish_time - start_time;

        tsc_to_bus_ratio = oneshot_time / deadline_time;
        ASSERT(tsc_to_bus_ratio != 0, "tsc-to-bus ratio is zero");
    }
}

void clear_irq_count()
{
    irq_count = 0;
}

static void lapic_irq_handler(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
}

static void counting_irq_handler(intr_regs*)
{
    ++irq_count;
    if (irq_count < EXPECTED_IRQS) {
        send_eoi();
    }
}

static void measuring_irq_handler(intr_regs*)
{
    if (irq_count == 0) {
        start_time = rdtsc();
    }
    ++irq_count;
    if (irq_count == EXPECTED_IRQS) {
        finish_time = rdtsc();
    }
    send_eoi();
}

/**
 * Waits for interrupts without performing VM exits (i.e., no HLT).
 */
void wait_for_interrupts(irq_handler_t handler, uint32_t irqs_expected)
{
    clear_irq_count();

    irq_handler::guard _(handler);

    enable_interrupts();
    write_lvt_mask(lvt_entry::TIMER, 0);

    while (irq_count < irqs_expected) {
    }

    write_lvt_mask(lvt_entry::TIMER, 1);
    disable_interrupts();

    send_eoi();
}

void drain_periodic_timer_irqs()
{
    enable_interrupts_for_single_instruction();
    while (irq_info.valid) {
        irq_info.reset();
        send_eoi();
        enable_interrupts_for_single_instruction();
    }
}

/**
 * Tests that periodic timer interrupts are delivered, even if the guest doesn't
 * perform a VM exit at the time the interrupt source is asserted.
 *
 * TL;DR: This tests that VMMs poke the vCPU.
 */
TEST_CASE(timer_mode_periodic_should_cycle)
{
    irq_handler::guard _(lapic_irq_handler);

    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::PERIODIC, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::MASKED });

    write_divide_conf(1);
    write_to_register(LAPIC_INIT_COUNT, TIMER_INIT_COUNT);

    wait_for_interrupts(counting_irq_handler, EXPECTED_IRQS);

    BARETEST_ASSERT((irq_count == EXPECTED_IRQS));

    drain_periodic_timer_irqs();
}

uint64_t measure_timer_period()
{
    stop_lapic_timer();

    write_to_register(LAPIC_INIT_COUNT, TIMER_INIT_COUNT);
    wait_for_interrupts(measuring_irq_handler, EXPECTED_IRQS);

    stop_lapic_timer();
    drain_periodic_timer_irqs();

    return finish_time - start_time;
}

// This test case failed sporadically on the T490 and is disabled for now. See:
// https://gitlab.vpn.cyberus-technology.de/supernova-core/supernova-core/-/issues/872
TEST_CASE_CONDITIONAL(higher_divide_conf_should_lead_to_slower_cycles, false)
{
    irq_handler::guard _(lapic_irq_handler);

    std::vector<uint64_t> total_times;
    drain_periodic_timer_irqs();

    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::PERIODIC, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::MASKED });

    for (uint32_t conf = 1; conf <= 128; conf *= 2) {
        write_divide_conf(conf);
        total_times.push_back(measure_timer_period());
    }

    // if we have a divide configuration of 1, it should be two times faster than a divide configuration of 2. So i want
    // a factor around 2, and i multiply by 1000 to have a better accuracy
    const cbl::interval factor_range(1900, 2101);
    bool success = true;
    for (uint32_t i = 0; i < total_times.size() - 1; ++i) {
        uint32_t factor = (total_times.at(i + 1) * 1000) / total_times.at(i);
        success = factor_range.contains(factor) ? success : false;
    }

    // printing the factors for debugging
    if (not success) {
        info("The test failed, some information for debugging: ");
        for (uint32_t i = 0; i < total_times.size() - 1; ++i) {
            uint32_t factor = (total_times.at(i + 1) * 1000) / total_times.at(i);
            info("  factor \t{} is \t{}", i, factor);
        }
    }

    BARETEST_ASSERT(success);
}

TEST_CASE_CONDITIONAL(timer_mode_tsc_deadline_should_send_irqs_on_specific_time, supports_tsc_deadline_mode())
{
    irq_handler::guard handler_guard(lapic_irq_handler);
    lvt_guard lvt_guard(lvt_entry::TIMER, MAX_VECTOR, lvt_timer_mode::DEADLINE);

    for (uint32_t i = 0; i < 10; ++i) {
        uint64_t deadline = rdtsc() + (1 << i);

        wrmsr(msr::IA32_TSC_DEADLINE, deadline);
        enable_interrupts_and_halt();

        uint64_t irq_time = rdtsc();
        disable_interrupts();
        send_eoi();

        // check that the interrupt isnt too early and not too late
        BARETEST_ASSERT((irq_time >= deadline));
        BARETEST_ASSERT((irq_time <= deadline + DEADLINE_OFFSET));
    }
}

TEST_CASE_CONDITIONAL(deadlines_in_the_past_should_produce_interrupts_immediately, supports_tsc_deadline_mode())
{
    irq_handler::guard handler_guard(lapic_irq_handler);
    lvt_guard lvt_guard(lvt_entry::TIMER, MAX_VECTOR, lvt_timer_mode::DEADLINE);

    for (uint32_t i = 0; i < 10; ++i) {
        uint64_t deadline = rdtsc() - (1 << i);
        irq_info.reset();

        wrmsr(msr::IA32_TSC_DEADLINE, deadline);

        // The lapic needs some time.
        // How much? We do not know. It depends on the system.
        // We have observed 64 TSC ticks to be insufficient on some systems.
        // Try with small increments until it works or give up after an
        // unreasonable waiting time.
        const uint64_t maximum_grace_period = 4096;
        uint64_t elapsed;
        uint64_t start = rdtsc();

        do {
            elapsed = rdtsc() - start;
            // The assumption is that the interrupt is now already pending and will
            // be delivered immediately.
            enable_interrupts_for_single_instruction();
        } while (!irq_info.valid && (elapsed < maximum_grace_period));

        info("\"Immediate\" interrupt delivery took about {} cycles.", elapsed);
        BARETEST_ASSERT(elapsed < maximum_grace_period);
        BARETEST_ASSERT(irq_info.valid);
        BARETEST_ASSERT((static_cast<uint32_t>(irq_info.vec) == MAX_VECTOR));

        send_eoi();
    }
}

TEST_CASE_CONDITIONAL(switch_from_deadline_to_oneshot_should_disarm_the_timer, supports_tsc_deadline_mode())
{
    irq_handler::guard handler_guard(lapic_irq_handler);
    lvt_guard lvt_guard(lvt_entry::TIMER, MAX_VECTOR, lvt_timer_mode::DEADLINE);
    irq_info.reset();

    enable_interrupts();

    // i chose a high deadline, so i have enough time to switch from deadline to oneshot mode
    uint64_t deadline = rdtsc() + 4 * DEADLINE_OFFSET;

    wrmsr(msr::IA32_TSC_DEADLINE, deadline);
    write_lvt_timer_mode(lvt_entry::TIMER, lvt_timer_mode::ONESHOT);

    ASSERT(rdtsc() < deadline, "Assumption broken");
    BARETEST_ASSERT((read_from_register(LAPIC_CURR_COUNT) == 0));

    // the 512 is there to give the interrupt some time
    while (rdtsc() <= deadline + 512) {
    };

    BARETEST_ASSERT(not irq_info.valid);

    disable_interrupts();
}

TEST_CASE_CONDITIONAL(switch_from_periodic_to_deadline_should_disarm_the_timer, supports_tsc_deadline_mode())
{
    irq_handler::guard handler_guard(lapic_irq_handler);
    lvt_guard lvt_guard(lvt_entry::TIMER, MAX_VECTOR, lvt_timer_mode::PERIODIC);
    irq_info.reset();

    // i need some time to change the lvt mode, so i choose a high div count
    stop_lapic_timer();
    write_divide_conf(1);

    // deadline offset because i need a large value
    write_to_register(LAPIC_INIT_COUNT, DEADLINE_OFFSET);
    write_lvt_timer_mode(lvt_entry::TIMER, lvt_timer_mode::DEADLINE);
    uint64_t start = rdtsc();

    // again, the * 2 is there to give the interrupt some time
    while (rdtsc() <= start + DEADLINE_OFFSET * (tsc_to_bus_ratio * 2)) {
    };

    if (irq_info.valid) {
        info("Test failed, got irq {}.", static_cast<uint32_t>(irq_info.vec))
    }
    BARETEST_ASSERT(not irq_info.valid);

    disable_interrupts();
}

// The Intel SDM is mentions switches from periodic/oneshot mode to/from
// TSC deadline mode. But it does not explicitly state the behaviour of
// switches from oneshot to periodic and vice versa.
TEST_CASE(switch_from_oneshot_to_periodic_does_not_disarm_the_timer)
{
    irq_handler::guard _(lapic_irq_handler);

    // Start in oneshot mode.
    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::ONESHOT, .mask = lvt_mask::MASKED });

    // Start the timer. Take the `DEADLINE_OFFSET` so that the timer interrupt
    // will (hopefully) not trigger before we switch modes.
    write_to_register(LAPIC_INIT_COUNT, DEADLINE_OFFSET);

    const auto curr_count_in_oneshot = read_from_register(LAPIC_CURR_COUNT);
    write_lvt_timer_mode(lvt_entry::TIMER, lvt_timer_mode::PERIODIC);
    const auto curr_count_in_periodic = read_from_register(LAPIC_CURR_COUNT);

    // If the current count is at 0, the oneshot timer already expired.
    // That means timeout value chosen in the test was too small and the
    // subsequent assertions will fail.
    BARETEST_ASSERT(curr_count_in_periodic != 0);

    // A mode switch should not affect the current counting (the timer should
    // not be reset).
    BARETEST_ASSERT(curr_count_in_oneshot >= curr_count_in_periodic);

    // Wait for the current counter to expire.
    wait_for_interrupts(counting_irq_handler, 1);
    BARETEST_ASSERT((irq_count == 1));

    // Ensure that we are in periodic mode by waiting for at least 2 more
    // interrupts.
    wait_for_interrupts(counting_irq_handler, 2);
    BARETEST_ASSERT((irq_count == 2));

    disable_interrupts();
}

TEST_CASE(switch_from_oneshot_to_periodic_after_oneshot_expired_does_not_rearm_timer)
{
    irq_handler::guard _(lapic_irq_handler);

    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::ONESHOT, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::MASKED });

    write_to_register(LAPIC_INIT_COUNT, TIMER_INIT_COUNT);
    wait_for_interrupts(counting_irq_handler, 1);

    // Since the timer expired, the current count is set to 0.
    // When the value remains 0 after a mode switch, the timer did not
    // start again.
    BARETEST_ASSERT((read_from_register(LAPIC_CURR_COUNT) == 0));
    write_lvt_timer_mode(lvt_entry::TIMER, lvt_timer_mode::PERIODIC);
    BARETEST_ASSERT((read_from_register(LAPIC_CURR_COUNT) == 0));
}

TEST_CASE(switch_from_periodic_to_oneshot_eventually_stops_timer)
{
    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::PERIODIC, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::MASKED });

    // Ensure the periodic timer ticks multiple times.
    write_to_register(LAPIC_INIT_COUNT, TIMER_INIT_COUNT);
    wait_for_interrupts(counting_irq_handler, 2);

    // Drain interrupts to fetch any additional timer IRQs which
    // may have been triggered while the timer was unmasked.
    drain_periodic_timer_irqs();

    // After switching to oneshot mode, there will be only a single interrupt
    // which we can't afford to miss. Hence, we need to unmask the interrupt
    // at the same time as switching modes which means we need to set up the
    // handler first.
    irq_handler::guard _(lapic_irq_handler);
    irq_info.reset();
    write_lvt_entry(lvt_entry::TIMER, { .vector = MAX_VECTOR, .timer_mode = lvt_timer_mode::ONESHOT, .dlv_mode = lvt_dlv_mode::FIXED, .mask = lvt_mask::UNMASKED });

    enable_interrupts();
    // Wait for at one interrupt.
    while (!irq_info.valid) {
    }
    disable_interrupts();

    // Make sure the timer has stopped.
    BARETEST_ASSERT((read_from_register(LAPIC_CURR_COUNT) == 0));
}

BARETEST_RUN;
