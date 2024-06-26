// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <stdint.h>

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/statistics.hpp>
#include <toyos/x86/x86asm.hpp>

using namespace lapic_test_tools;
void drain_interrupts();

static const uint8_t BENCH_VEC{ 213 };
static std::atomic<bool> irq_fired{ false };
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
    const unsigned REPETITIONS{ 1000 };

    drain_interrupts();
    disable_interrupts();

    irq_handler::guard _(lapic_irq_handler_benchmark);

    for (unsigned run{ 0 }; run < REPETITIONS; ++run) {
        send_self_ipi(BENCH_VEC);

        bench_ipi.start();
        enable_interrupts_for_single_instruction();

        BARETEST_ASSERT(irq_fired.exchange(false));
    }

    BENCHMARK_RESULT("self_ipi_cycles", bench_ipi.result().avg(), "cycles");
}

TEST_CASE(benchmark_read_lapic_id_cycles)
{
    const unsigned REPETITIONS{ 10000 };

    auto data{ statistics::measure_cycles([]() { [[maybe_unused]] uint32_t apic_id = read_from_register(LAPIC_ID); },
                                          REPETITIONS) };

    BENCHMARK_RESULT("read_lapic_id_cycles", data.avg(), "cycles");
}
