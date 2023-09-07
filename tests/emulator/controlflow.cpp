/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/testhelper/debugport_interface.h>

#include <compiler.hpp>
#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/usermode.hpp>

static usermode_helper um;

#define ASM_EMUL_ONCE "mov %[dbgport_val], %%rax; outb %[dbgport];"
#define CHECK_EMUL(insn) "test %[emul_" #insn "], %[emul_" #insn "]; jz 42f;" ASM_EMUL_ONCE "42:"

struct near_call_params
{
    bool emulate_call, emulate_ret;
};

static bool near_call_ret(const near_call_params&& params)
{
    volatile bool success_relative {false}, success_absolute_reg {false}, success_absolute_mem {false};
    uintptr_t call_target {0};
    asm volatile(
        // Near relative
        "sub $16, %%rsp;" CHECK_EMUL(
            call) "call 1f;"
                  "jmp 2f;"
                  "1: movb $1, %[success_rel];" CHECK_EMUL(
                      ret) "   ret $16;"
                           "2:"
                           // Near absolute, register operand
                           "lea 3f, %%rcx;" CHECK_EMUL(
                               call) "call *%%rcx;"
                                     "jmp 4f;"
                                     "3: movb $1, %[success_abs_reg];" CHECK_EMUL(
                                         ret) "   ret;"
                                              "4:"
                                              // Near absolute, memory operand
                                              "lea 5f, %%rcx;"
                                              "mov %%rcx, %[call_target];" CHECK_EMUL(
                                                  call) "call *%[call_target];"
                                                        "jmp 6f;"
                                                        "5: movb $1, %[success_abs_mem];" CHECK_EMUL(ret) "   ret;"
                                                                                                          "6:"
        : [success_rel] "=&r"(success_relative), [success_abs_reg] "=&r"(success_absolute_reg),
          [success_abs_mem] "=&r"(success_absolute_mem), [call_target] "=&m"(call_target)
        : [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "i"(DEBUGPORT_EMUL_ONCE), [emul_call] "q"(params.emulate_call),
          [emul_ret] "q"(params.emulate_ret)
        : "rax", "rcx");
    return success_relative and success_absolute_reg and success_absolute_mem;
}

TEST_CASE(call_ret_near)
{
    BARETEST_VERIFY(near_call_ret({.emulate_call = true, .emulate_ret = false}));
    BARETEST_VERIFY(near_call_ret({.emulate_call = false, .emulate_ret = true}));
    BARETEST_VERIFY(near_call_ret({.emulate_call = true, .emulate_ret = true}));
}

namespace
{
template <typename T>
struct __PACKED__ far_ptr
{
    T off;
    uint16_t sel;
};
} // namespace

TEST_CASE(call_far_pointer)
{
    static volatile far_ptr<uint32_t> target_32bit;
    static volatile far_ptr<uint64_t> target_64bit;

    volatile bool success_32bit {false}, success_64bit {false};
    asm volatile("mov %%cs, %%cx;"
                 "mov %%cx, %[target_32bit_sel];"
                 "mov %%cx, %[target_64bit_sel];"
                 "lea 1f, %%rcx;"
                 "mov %%ecx, %[target_32bit_off];"
                 "lea 3f, %%rcx;"
                 "mov %%rcx, %[target_64bit_off];"
                 // 32-bit operand size
                 "mov %[dbgport_val], %%rax;"
                 "outb %[dbgport];"
                 "lcall *%[target_32bit];"
                 "jmp 2f;"
                 "1: movb $1, %[success_32bit];"
                 "   lretl;"
                 "2:"
                 // 64-bit operand size
                 "mov %[dbgport_val], %%rax;"
                 "outb %[dbgport];"
#ifdef __clang__
                 "lcallq *%[target_64bit];"
#else
                 "rex64 lcall *%[target_64bit];"
#endif
                 "jmp 4f;"
                 "3: movb $1, %[success_64bit];"
                 "   lretq;"
                 "4:"
                 : [target_32bit_sel] "=&m"(target_32bit.sel), [target_32bit_off] "=&m"(target_32bit.off),
                   [target_64bit_sel] "=&m"(target_64bit.sel), [target_64bit_off] "=&m"(target_64bit.off),
                   [success_32bit] "=&r"(success_32bit), [success_64bit] "=&r"(success_64bit)
                 : [target_32bit] "m"(target_32bit), [target_64bit] "m"(target_64bit), [dbgport] "i"(DEBUG_PORTS.a),
                   [dbgport_val] "i"(DEBUGPORT_EMUL_ONCE)
                 : "rax", "rcx");
    BARETEST_VERIFY(success_32bit and success_64bit);
}

TEST_CASE(call_far_conforming)
{
    static volatile far_ptr<uint64_t> target;
    volatile uint16_t resulting_cs {0};

    asm volatile("mov %%cs, %0" : "=r"(target.sel));

    // Enable conforming bit in GDT entry
    auto gdtr = get_current_gdtr();
    auto gdt_cs = reinterpret_cast<volatile x86::gdt_entry*>(gdtr.base + target.sel);
    gdt_cs->set_conforming(true);

    um.enter_sysret();

    asm volatile("lea 1f, %%rcx;"
                 "mov %%rcx, %[target_off];"
                 "mov %[dbgport_val], %%rax;"
                 "outb %[dbgport];"
#ifdef __clang__
                 "lcallq *%[target];"
#else
                 "rex64 lcall *%[target];"
#endif
                 "jmp 2f;"
                 "1: mov %%cs, %[resulting_cs];"
                 "   lretq;"
                 "2:"
                 : [target_off] "=&m"(target.off), [resulting_cs] "=&r"(resulting_cs)
                 : [target] "m"(target), [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "i"(DEBUGPORT_EMUL_ONCE)
                 : "rax", "rcx");

    um.leave_syscall();

    gdt_cs->set_conforming(false);

    BARETEST_VERIFY(resulting_cs == (target.sel | 0x3));
}

TEST_CASE(lea)
{
    struct result
    {
        uint16_t addr16;
        uint32_t addr32;
        uint64_t addr64;
        uint64_t addr64_pfx;

        bool operator==(const result& o) const
        {
            return addr16 == o.addr16 and addr32 == o.addr32 and addr64 == o.addr64 and addr64_pfx == o.addr64_pfx;
        }
    };
    result result_native, result_emul;
    asm volatile("mov $0x1FEE00080, %%rbx;"
                 "lea (%%rbx), %[addr_native16];"
                 "lea (%%rbx), %[addr_native32];"
                 "lea (%%rbx), %[addr_native64];"
                 "lea (%%ebx), %[addr_native64_pfx];"
                 "mov %[dbgport_val], %%rax;"
                 "outb %[dbgport];"
                 "lea (%%rbx), %[addr_emul16];"
                 "outb %[dbgport];"
                 "lea (%%rbx), %[addr_emul32];"
                 "outb %[dbgport];"
                 "lea (%%rbx), %[addr_emul64];"
                 "outb %[dbgport];"
                 "lea (%%ebx), %[addr_emul64_pfx];"
                 : [addr_native16] "=r"(result_native.addr16), [addr_emul16] "=r"(result_emul.addr16),
                   [addr_native32] "=r"(result_native.addr32), [addr_emul32] "=r"(result_emul.addr32),
                   [addr_native64] "=r"(result_native.addr64), [addr_emul64] "=r"(result_emul.addr64),
                   [addr_native64_pfx] "=r"(result_native.addr64_pfx), [addr_emul64_pfx] "=r"(result_emul.addr64_pfx)
                 : [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "i"(DEBUGPORT_EMUL_ONCE)
                 : "rax", "rbx");
    BARETEST_VERIFY(result_native == result_emul);
}

TEST_CASE(je)
{
    // Test once where we want to jump
    bool jump_performed {false};
    asm volatile("mov $0, %[output];"
                 "jmp 2f;"
                 "1:"
                 "mov $1, %[output];"
                 "jmp 3f;"
                 "2:"
                 "cmp $1, %[jump];"
                 "outb %[dbgport];"
                 "je 1b;"
                 "3:"
                 : [output] "=&r"(jump_performed)
                 : [jump] "r"(1), [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "a"(DEBUGPORT_EMUL_ONCE));
    BARETEST_VERIFY(jump_performed);

    // And another time with a false jump condition
    bool jump_not_performed {false};
    asm volatile("mov $0, %[output];"
                 "cmp $1, %[skip];"
                 "outb %[dbgport];"
                 "je 1f;"
                 "mov $1, %[output];"
                 "1:"
                 : [output] "=&r"(jump_not_performed)
                 : [skip] "r"(0), [dbgport] "i"(DEBUG_PORTS.a), [dbgport_val] "a"(DEBUGPORT_EMUL_ONCE));
    BARETEST_VERIFY(jump_not_performed);
}
