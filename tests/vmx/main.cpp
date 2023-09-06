/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <baretest/baretest.hpp>
#include <idt.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <lapic_test_tools.hpp>
#include <logger/trace.hpp>

#define TEST_CASE_VMX(NAME, EXCEPTION, FUNCTION)                                                                       \
    TEST_CASE(NAME)                                                                                                    \
    {                                                                                                                  \
        irq_handler::guard _(irq_handle);                                                                              \
        irq_info.reset();                                                                                              \
        if (setjmp(jump_buffer) == 0) {                                                                                \
            FUNCTION                                                                                                   \
        }                                                                                                              \
        BARETEST_ASSERT((irq_info.vec == static_cast<uint32_t>(EXCEPTION)));                                           \
    }

#define TEST_CASE_VMX_UD(NAME, FUNCTION) TEST_CASE_VMX(NAME, exception::UD, FUNCTION);

using namespace lapic_test_tools;
using namespace x86;

jmp_buf jump_buffer;

static irqinfo irq_info;
struct m128
{
    uint64_t val1 {0}, val2 {0};
};

static void irq_handle(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    longjmp(jump_buffer, 1);
}

TEST_CASE_VMX_UD(vmcall_should_invoke_invalid_opcode_exception, { asm volatile("vmcall"); });

TEST_CASE_VMX_UD(vmclear_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmclear %0" ::"m"(dummy));
});

TEST_CASE_VMX_UD(vmptrld_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmptrld %0" ::"m"(dummy));
});

TEST_CASE_VMX_UD(vmlaunch_should_invoke_invalid_opcode_exception, { asm volatile("vmlaunch"); });

TEST_CASE_VMX_UD(vmresume_should_invoke_invalid_opcode_exception, { asm volatile("vmresume"); });

TEST_CASE_VMX_UD(invept_should_invoke_invalid_opcode_exception, {
    m128 desc;
    asm volatile("invept %0, %1" ::"m"(desc), "r"(1UL));
});

TEST_CASE_VMX_UD(invvpid_should_invoke_invalid_opcode_exception, {
    m128 desc;
    asm volatile("invvpid %0, %1" ::"m"(desc), "r"(1UL));
});

TEST_CASE_VMX_UD(vmfunc_should_invoke_invalid_opcode_exception, { asm volatile("vmfunc"); });

TEST_CASE_VMX_UD(vmptrst_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmptrst %0" ::"m"(dummy));
});

TEST_CASE_VMX_UD(vmread_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmread %0, %1" ::"r"(1UL), "m"(dummy));
});

TEST_CASE_VMX_UD(vmwrite_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmwrite %0, %1" ::"m"(dummy), "r"(1UL));
});

TEST_CASE_VMX_UD(vmxoff_should_invoke_invalid_opcode_exception, { asm volatile("vmxoff"); });

TEST_CASE_VMX_UD(vmxon_should_invoke_invalid_opcode_exception, {
    uint64_t dummy {0};
    asm volatile("vmxon %0" ::"m"(dummy));
});

BARETEST_RUN;
