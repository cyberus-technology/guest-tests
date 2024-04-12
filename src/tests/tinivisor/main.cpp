/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86defs.hpp>

#include <toyos/tinivisor.hpp>

#include <toyos/util/math.hpp>

#include <toyos/testhelper/int_guard.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>

#include <toyos/baretest/baretest.hpp>

using namespace lapic_test_tools;

/**
 * \brief Helper for starting and stopping tinivisor
 */
class tinivisor_guard
{
 public:
    explicit tinivisor_guard(tinivisor& tini_)
        : tini(tini_)
    {
        tini.start();
    }
    ~tinivisor_guard()
    {
        tini.stop();
    }

 private:
    tinivisor& tini;
};

tinivisor tini;

/**
 * \brief Emulate CPUID.
 *
 * This exit handler for CPUID forwards the call to the host, toggles MMX bit and replies.
 */
void cpuid_toggle_mmx(vmcs::vmcs_t& vmcs, tinivisor::guest_regs& regs, x86::vmx_exit_reason /*reason*/, uint64_t /*exit_qual*/)
{
    if (regs.rax == CPUID_LEAF_FAMILY_FEATURES) {
        auto host_features{ cpuid(CPUID_LEAF_FAMILY_FEATURES) };
        regs.rdx = host_features.edx ^ LVL_0000_0001_EDX_MMX;
    }
    vmcs.adjust_rip();
}

/**
 * \brief Use CPUID to determine MMX support.
 *
 * \return MMX support
 */
inline bool mmx_supported()
{
    return (cpuid(CPUID_LEAF_FAMILY_FEATURES).edx & LVL_0000_0001_EDX_MMX) != 0;
}

// needed for IRQ test
void prologue()
{
    // disable PIC's interrupts
    mask_pic();
    // configure vector used by spurious interrupts
    write_spurious_vector(SPURIOUS_TEST_VECTOR);

    irq_handler::guard _(drain_irq);
    enable_interrupts_for_single_instruction();

    software_apic_enable();
}

TEST_CASE(tinivisor_cpuid_feature_hiding_works)
{
    tini.register_handler(x86::vmx_exit_reason::CPUID, cpuid_toggle_mmx);
    auto host_state{ mmx_supported() };

    bool guest_state = host_state;
    {
        tinivisor_guard _(tini);
        guest_state = mmx_supported();
    }

    BARETEST_ASSERT(host_state != guest_state);
}

TEST_CASE(tinivisor_disabling_tinivisor_works)
{
    auto host_state_before{ mmx_supported() };

    tini.register_handler(x86::vmx_exit_reason::CPUID, cpuid_toggle_mmx);
    {
        tinivisor_guard _(tini);
        auto guest_state{ mmx_supported() };
        BARETEST_ASSERT(host_state_before != guest_state);
    }

    BARETEST_ASSERT(host_state_before == mmx_supported());
}

static irqinfo irq_info;

static void handle_irq(intr_regs* regs)
{
    irq_info.record(regs->vector, regs->error_code);
    send_eoi();
}

TEST_CASE(tinivisor_self_ipi_is_delivered_in_vmx_nonroot_mode)
{
    tinivisor_guard tini_guard(tini);

    irq_handler::guard _(handle_irq);
    irq_info.reset();

    constexpr static uint8_t VECTOR{ 33 };
    send_self_ipi(VECTOR);

    enable_interrupts_and_halt();
    disable_interrupts();
    BARETEST_ASSERT(irq_info.valid);
    BARETEST_ASSERT(irq_info.vec == VECTOR);
}

/**
 * The next test should checks if io port access works via writing and reading the debug port .
 */
TEST_CASE(tinivisor_io_port_access_works)
{
    tinivisor_guard _(tini);
    constexpr uint8_t DEBUG_PORT{ 0x80 };

    outb(DEBUG_PORT, 0xff);
    inb(DEBUG_PORT);

    /*
     * The tinivisor will throw an assertion, if in and out instructions fail, so
     * We have no Condition to check here.
     */
    BARETEST_ASSERT(true);
}

void cr4_exit(vmcs::vmcs_t& vmcs, tinivisor::guest_regs& regs, x86::vmx_exit_reason /*reason*/, uint64_t exit_qual)
{
    auto reg = (exit_qual >> 8) & 0xf;
    auto val = 0u;
    switch (reg) {
        case 0:
            val = regs.rax;
            break;
        case 1:
            val = regs.rcx;
            break;
        case 2:
            val = regs.rdx;
            break;
        case 3:
            val = regs.rbx;
            break;
        default:
            PANIC("Unknown register");
    }

    uint64_t cr4_cur = get_cr4() & ~math::mask_from(x86::cr4::VMXE);
    uint64_t good1 = cr4_cur | math::mask_from(x86::cr4::PGE);
    uint64_t good2 = cr4_cur & ~math::mask_from(x86::cr4::PGE);

    if (val != good1 and val != good2) {
        warning("Invalid write to CR4: {#x}", val);
    }
    BARETEST_ASSERT(val == good1 or val == good2);

    vmcs.write(x86::vmcs_encoding::CR4_READ_SHADOW, val);
    vmcs.write(x86::vmcs_encoding::GUEST_CR4, val | math::mask_from(x86::cr4::VMXE));
    vmcs.adjust_rip();
}

TEST_CASE(tinivisor_nested_guest_should_never_see_vmxe_in_cr4)
{
    tini.reset();

    tinivisor_guard _(tini);

    tini.register_handler(x86::vmx_exit_reason::CR_ACCESS, cr4_exit);

    for (unsigned rounds{ 100000 }; rounds > 0; rounds--) {
        set_cr4(get_cr4() | math::mask_from(x86::cr4::PGE));
        set_cr4(get_cr4() & ~math::mask_from(x86::cr4::PGE));
        asm volatile("pause");
    }
}

/**
 * \brief Check that callee saved registers are preserved by tinivisor member functions.
 *
 * When starting or stopping tinivisor, execution contexts are switched. This helper can be
 * used to assure that callee saved registers are correctly handled.
 *
 * This function is going to modify the base pointer in inline assembly. Because the base pointer can not be put in
 * clobber list, it's value to be saved and manually. One option is to use the stack - push at the beginning, pop
 * at the end. But when the compiler uses stack pointer relative addresses for accessing operands, a stack pointer
 * modification won't work. Hence, the base pointer's value is stored in a variable. A local variable does not work
 * because we can not prevent the compiler from using a base-pointer relative address when restoring the saved base
 * pointer. Hence, a static variable holds the base pointer.
 *
 * \param tinivisor::*func pointer to tinivisor member function which will be called on the global tinivisor object
 *
 * \return whether registers have been correctly preserved
 */
bool callee_saved_registers_preserved_on_call(void (tinivisor::*func)())
{
    static constexpr uint64_t BEFORE{ 0xfefe };
    std::array<uint64_t, 6> regs{ 0 };
    static uint64_t saved_rbp;

    asm volatile(
        // going to call member function - first (hidden) parameter is the this-pointer - to tinivisor object
        "mov %[_tini], %%rdi;"

        // assign initial value to all callee saved registers
        "mov %[_input], %%rbx;"
        "mov %[_input], %%r12;"
        "mov %[_input], %%r13;"
        "mov %[_input], %%r14;"
        "mov %[_input], %%r15;"

        // copy address of the function we shall call to temporary register because we will be unable
        // to use the base pointer to access local variables at the point of the call
        "mov %[_method], %%rax;"

        // save current base pointer
        "mov %%rbp, %[_saved_rbp];"

        // assign initial value to base pointer
        // make sure to access operands only after the saved value has been restored
        "mov %[_input], %%rbp;"

        // use register holding saved address to call member function
        "call *%%rax;"

        // save current base pointer value without using an operand
        "mov %%rbp, %%rax;"

        // restore base pointer - it is safe to use local variables after this instruction
        "mov %[_saved_rbp], %%rbp;"

        // write values to local variables
        "mov %%rbx, %[_rbx];"
        "mov %%rax, %[_rbp];"  // use temporary copy
        "mov %%r12, %[_r12];"
        "mov %%r13, %[_r13];"
        "mov %%r14, %[_r14];"
        "mov %%r15, %[_r15];"
        : [_rbp] "=rm"(regs[0]), [_rbx] "=rm"(regs[1]), [_r12] "=rm"(regs[2]), [_r13] "=rm"(regs[3]), [_r14] "=rm"(regs[4]), [_r15] "=rm"(regs[5]),
          [_saved_rbp] "=&rm"(saved_rbp)  // written before _input is read - needs early clobber
        : [_method] "m"(func), [_input] "i"(BEFORE), [_tini] "rm"(&tini)
        // clobber registers changed here and the ones which the called function may modify
        : "rax", "rbx", "rcx", "rdx", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rdi");

    return std::all_of(regs.cbegin(), regs.cend(), [](uint64_t val) { return val == BEFORE; });
}

TEST_CASE(tinivisor_start_preserves_callee_saved_regs)
{
    BARETEST_ASSERT(callee_saved_registers_preserved_on_call(&tinivisor::start));
    tini.stop();
}

TEST_CASE(tinivisor_stop_preserves_callee_saved_regs)
{
    tini.start();
    BARETEST_ASSERT(callee_saved_registers_preserved_on_call(&tinivisor::stop));
}

BARETEST_RUN;
