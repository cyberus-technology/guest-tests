/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <atomic>
#include <stdint.h>

#include <baretest/baretest.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <lapic_test_tools.hpp>
#include <statistics/statistics.hpp>
#include <x86asm.hpp>

using namespace lapic_test_tools;
void drain_interrupts();

static const uint8_t BENCH_VEC {213};
static std::atomic<bool> irq_fired {false};
statistics::cycle_acc bench_ipi;

static void lapic_irq_handler_benchmark(intr_regs* regs)
{
    bench_ipi.stop();

    PANIC_UNLESS(regs->vector == BENCH_VEC, "wrong vector");
    PANIC_ON(irq_fired.exchange(true), "irq_fired was true");

    send_eoi();
}

TEST_CASE(benchmark_interrupt_delivery_latency)
{
    const unsigned REPETITIONS {1000};

    drain_interrupts();
    disable_interrupts();

    irq_handler::guard _(lapic_irq_handler_benchmark);

    for (unsigned run {0}; run < REPETITIONS; ++run) {
        send_self_ipi(BENCH_VEC);

        bench_ipi.start();
        enable_interrupts_for_single_instruction();

        BARETEST_ASSERT(irq_fired.exchange(false));
    }

    BENCHMARK_RESULT("self_ipi_cycles", bench_ipi.result().avg(), "cycles");
}

TEST_CASE(benchmark_read_lapic_id_cycles)
{
    const unsigned REPETITIONS {10000};

    auto data {statistics::measure_cycles([]() { [[maybe_unused]] uint32_t apic_id = read_from_register(LAPIC_ID); },
                                          REPETITIONS)};

    BENCHMARK_RESULT("read_lapic_id_cycles", data.avg(), "cycles");
}
