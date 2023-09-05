/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <idt.hpp>
#include <irq_handler.hpp>
#include <irqinfo.hpp>
#include <x86asm.hpp>

#include <slvm/api.h>

#include <baretest/baretest.hpp>

using x86::exception;

namespace
{

irqinfo irq_info;

void irq_handler_fn(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    irq_info.fixup(regs);
}

} // namespace

TEST_CASE(unavailable_slvm_api_returns_ud)
{
    irqinfo::func_t fixup = [](intr_regs* regs) { regs->rip = regs->rbx; };
    uint64_t fn {SLVMAPI_VMCALL_VIRT_CHECK_PRESENCE};
    uint64_t ignore {0};

    irq_handler::set(irq_handler_fn);

    irq_info.reset();
    irq_info.fixup_fn = fixup;

    asm volatile("lea 1f, %%rbx;"
                 "vmcall;"
                 "1:;"
                 : "+a"(fn), "+" SLVMAPI_VMCALL_PARAM1(ignore), "+" SLVMAPI_VMCALL_PARAM2(ignore),
                   "+" SLVMAPI_VMCALL_PARAM3(ignore), "+" SLVMAPI_VMCALL_PARAM4(ignore)
                 :
                 : "rbx");

    BARETEST_ASSERT(fn == SLVMAPI_VMCALL_VIRT_CHECK_PRESENCE);
    BARETEST_ASSERT(irq_info.valid and static_cast<exception>(irq_info.vec) == exception::UD);
}

BARETEST_RUN;
