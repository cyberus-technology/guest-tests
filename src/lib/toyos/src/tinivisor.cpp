/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cstring>

#include <assert.h>

#include <toyos/util/math.hpp>
#include <toyos/util/trace.hpp>

#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/segmentation.hpp>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86defs.hpp>

#include <toyos/tinivisor.hpp>

using encoding = x86::vmcs_encoding;

extern char exit_handler_low_level[];
EXTERN_C void restore_arch_state_and_jump(tinivisor::guest_regs& regs, uint64_t rsp);

tinivisor::handler_container_t tinivisor::handlers{ tinivisor::default_exit_handler };

void tinivisor::reset()
{
    std::fill(tinivisor::handlers.begin(), tinivisor::handlers.end(), tinivisor::default_exit_handler);
}

void tinivisor::default_exit_handler(vmcs::vmcs_t& /*vmcs*/, tinivisor::guest_regs& /*regs*/, x86::vmx_exit_reason reason, uint64_t /*exit_qual*/)
{
    PANIC("no exit handler registered for exit {}", to_underlying(reason));
}

/**
 * \brief Handler for vmcall. Used to disable tinivisor.
 *
 * Like other exit handlers, this function is called by the low level entry point
 * after guest arch state has been saved to a predefined location.
 * Hence, the host's exit handler stack is currently in use.
 * When disabling tinivisor, control is returned to the caller of tinivisor::stop.
 * The low level function called here switches to the guest stack and restores the register state.
 * It then jumps to disable_vmx and does not return.
 */
[[noreturn]] void handle_vmcall(vmcs::vmcs_t& vmcs, tinivisor::guest_regs& regs, x86::vmx_exit_reason /*reason*/, uint64_t /*exit_qual*/)
{
    restore_arch_state_and_jump(regs, vmcs.read(encoding::GUEST_RSP));
    __UNREACHED__;
}

void tinivisor::start()
{
    lock_feature_control_msr();
    assert(is_vmx_supported());

    // prepare clean state for multiple invocations of tinivisor::start (they all share the VMCS)
    vmcs.clear();

    // enable vmx
    set_cr4(get_cr4() | math::mask_from(x86::cr4::VMXE));

    apply_vmx_enforced_control_register_bits();

    auto rev_id{ get_vmcs_revision_id() };
    // Write VMCS revision identifier to vmxon region before calling vmxon
    // (Intel SDM, Vol. 3, 24.11.5 VMXON Region).
    vmxon_page.rev_id = rev_id;
    vmxon(uintptr_t(&vmxon_page));

    vmclear(uintptr_t(&vmcs));

    // Before calling vmptrld write VMCS revision identifier because the
    // instruction fails otherwise (Intel SDM, Vol. 3, 24.2 - Format of the VMCS Region).
    vmcs.set_rev_id(rev_id);
    vmptrld(uintptr_t(&vmcs));

    // fill VMCS - order of the following calls does not matter
    disable_secondary_execution_controls();

    clone_segments();
    clone_control_registers();

    // clone rflags
    vmcs.write(encoding::GUEST_RFLAGS, get_rflags());

    // no linked vmcs
    vmcs.write(encoding::VMCS_LINK_PTR, ~0ul);

    configure_64_bit_host();
    configure_64_bit_guest();
    configure_exits();

    // register_handler does not allow a VMCALL handler and we register it manually
    handlers[to_underlying(x86::vmx_exit_reason::VMCALL)] = handle_vmcall;

    /*
     * We are going to switch to vmx non root mode with vmlaunch
     * Thereafter, we continue with the guest register state from our VMCS.
     * It includes (see Vol. 3, Chapter 24.4.1):
     *      - cr0, cr3, cr4
     *      - dr7
     *      - rsp, rip, rflags
     *      - for cs, ss, ds, es, fs, gs, ldtr, tr, gdtr, idtr:
     *          selector, base, limit, access rights
     *      - selected msrs
     *      - smbase
     *
     * We prepared everything except for rsp and rip already.
     * To continue in the current function, we are going to set the instruction
     * pointer to a label in the assembly snippet below.
     * Our current stack pointer is copied to the VMCS.
     */
    bool switch_to_guest_failed;

    // let the compiler choose the best register to store our jump address in
    uint64_t dummy_start_addr;
    asm volatile("xor %[error], %[error];"

                 // copy stack pointer value to vmcs guest state
                 "vmwrite %%rsp, %[_vmcs_rsp];"
                 "jbe .error;"

                 // set instruction pointer in vmcs guest state
                 "lea .vmx_non_root_start, %[addr_reg];"
                 "vmwrite %[addr_reg], %[_vmcs_rip];"
                 "jbe .error;"

                 "vmlaunch;"
                 // continuing here means we did not switch to vmx non-root mode
                 // or one of the above commands failed and caused a jump
                 ".error:"
                 "inc %[error];"

                 // From this point on, RFLAGS has the value prepared in the VMCS.
                 // Keep this mind in case you want to perform a conditional statement.
                 ".vmx_non_root_start:"
                 : [error] "=&r"(switch_to_guest_failed),  // early output (modified before all inputs have been used)
                   [addr_reg] "=&r"(dummy_start_addr)      // early output
                 // without the upcoming casts the compiler wants to call the 32bit vmwrite
                 : [_vmcs_rsp] "r"(static_cast<uint64_t>(encoding::GUEST_RSP)), [_vmcs_rip] "r"(static_cast<uint64_t>(encoding::GUEST_RIP))
                 : "cc", "memory");

    assert(not switch_to_guest_failed);
}

void tinivisor::stop()
{
    // vmcall causes an exit. we set up the exit handler for vmcall
    // to continue at handle_vmcall.
    // handle_vmcall restores the register state and jumps to disable_vmx.
    asm volatile("vmcall; ud2a;");
    // Do not use __builtin_* here, it leads to aggressive optimization
    // and breaks the return path.
}

uint32_t tinivisor::get_vmcs_revision_id() const
{
    // Appendix A.1
    // Bits 30:0 contain the 31-bit VMCS revision identifier used by the processor.
    using constants = x86::vmx_basic_constants;

    auto vmx_basic{ static_cast<uint32_t>(rdmsr(x86::IA32_VMX_BASIC)) };
    return (vmx_basic & to_underlying(constants::VMCS_REVISION_ID_MASK))
           >> to_underlying(constants::VMCS_REVISION_ID_SHIFT);
}

void tinivisor::lock_feature_control_msr()
{
    // If the lock bit in the feature control MSR is clear, VMXON
    // causes a GP (Intel SDM Vol. 3, 23.7 - Enabling and Entering VMX Operation).
    //
    // Some BIOS leave the bit clear and we set have to set it manually.
    // This can only be done if the bit is not already set, because a
    // write to the "locked" MSR causes a GP (Intel SDM Vol. 4, Table 2-2 - IA-32 Architectural MSRs).
    auto feature_control{ rdmsr(x86::IA32_FEATURE_CONTROL) };
    if (not(feature_control & x86::IA32_FEATURE_CONTROL_LOCK)) {
        info("Locking feature controls.");
        feature_control |= x86::IA32_FEATURE_CONTROL_LOCK;
        wrmsr(x86::IA32_FEATURE_CONTROL, feature_control);
    }
}

bool tinivisor::is_vmx_supported() const
{
    auto cpuid_features{ cpuid(CPUID_LEAF_FAMILY_FEATURES) };
    bool cpuid_vmx_supported{ static_cast<bool>(cpuid_features.ecx & LVL_0000_0001_ECX_VMX) };

    // There are two settings in the feature control MSR:
    // - is VMXON in SMX operation enabled
    // - is VMXON outside SMX operation enabled
    // We assume to be outside of SMX and check therefore only the corresponding bit.
    auto feature_control{ rdmsr(x86::IA32_FEATURE_CONTROL) };
    bool vmxon_outside_smx_causes_gp{ not(feature_control & x86::IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX) };

    return cpuid_vmx_supported and not vmxon_outside_smx_causes_gp;
}

void tinivisor::apply_vmx_enforced_control_register_bits()
{
    // Intel SDM, Vol. 3, Appendix A.7 - VMX-FIXED BITS IN CR0:
    // If bit X is 1 in IA32_VMX_CR0_FIXED0, then that bit of CR0 is fixed to 1 in VMX operation.
    // Similarly, if bit X is 0 in IA32_VMX_CR0_FIXED1, then that bit of CR0 is fixed to 0 in VMX operation.
    uint64_t cr0_fixed_set{ rdmsr(x86::IA32_VMX_CR0_FIXED0) };
    // We use the inverted value of this MSR so that we can clear the bits which have to be 0
    // with common bit operations.
    uint64_t cr0_fixed_clr{ ~rdmsr(x86::IA32_VMX_CR0_FIXED1) };
    set_cr0((get_cr0() | cr0_fixed_set) & ~cr0_fixed_clr);

    // same for cr4 - Intel SDM, Vol. 3, Appendix A.8 - VMX-FIXED BITS IN CR4
    uint64_t cr4_fixed_set{ rdmsr(x86::IA32_VMX_CR4_FIXED0) };
    uint64_t cr4_fixed_clr{ ~rdmsr(x86::IA32_VMX_CR4_FIXED1) };
    set_cr4((get_cr4() | cr4_fixed_set) & ~cr4_fixed_clr);
}

void tinivisor::clone_segments()
{
    x86::descriptor_ptr gdtr{ get_current_gdtr() };

    auto cs{ clone_to_guest_CS(vmcs, { gdtr, get_cs() }) };
    auto ss{ clone_to_guest_SS(vmcs, { gdtr, get_ss() }) };
    auto ds{ clone_to_guest_DS(vmcs, { gdtr, get_ds() }) };
    auto es{ clone_to_guest_ES(vmcs, { gdtr, get_es() }) };
    auto fs{ clone_to_guest_FS(vmcs, { gdtr, get_fs() }) };
    auto gs{ clone_to_guest_GS(vmcs, { gdtr, get_gs() }) };
    auto tr{ clone_to_guest_TR(vmcs, { gdtr, x86::str() }) };
    auto idtr{ clone_to_guest_basic_IDTR(vmcs, get_current_idtr()) };

    clone_to_guest_LDTR(vmcs, { gdtr, x86::sldt() });
    clone_to_guest_basic_GDTR(vmcs, gdtr);

    vmcs.write(encoding::HOST_SEL_CS, cs.selector);
    vmcs.write(encoding::HOST_SEL_SS, ss.selector);
    vmcs.write(encoding::HOST_SEL_DS, ds.selector);
    vmcs.write(encoding::HOST_SEL_ES, es.selector);
    vmcs.write(encoding::HOST_SEL_FS, fs.selector);
    vmcs.write(encoding::HOST_BASE_FS, fs.base);
    vmcs.write(encoding::HOST_SEL_GS, gs.selector);
    vmcs.write(encoding::HOST_BASE_GS, gs.base);
    vmcs.write(encoding::HOST_SEL_TR, tr.selector);
    vmcs.write(encoding::HOST_BASE_TR, tr.base);
    vmcs.write(encoding::HOST_BASE_GDTR, gdtr.base);
    vmcs.write(encoding::HOST_BASE_IDTR, idtr.base);
}

void tinivisor::clone_control_registers()
{
    vmcs.write(encoding::GUEST_CR0, get_cr0());
    vmcs.write(encoding::GUEST_CR3, get_cr3());
    vmcs.write(encoding::GUEST_CR4, get_cr4());
    vmcs.write(encoding::CR0_READ_SHADOW, get_cr0());
    vmcs.write(encoding::CR4_READ_SHADOW, get_cr4() & ~uint64_t(x86::cr4::VMXE));
    vmcs.write(encoding::HOST_CR0, get_cr0());
    vmcs.write(encoding::HOST_CR3, get_cr3());
    vmcs.write(encoding::HOST_CR4, get_cr4());
}

void tinivisor::configure_exits()
{
    // Stack and instruction pointer for vm exits.
    // RSP (current top most element) is set to just behind the reserved memory
    // region. This is fine because the exit handler's first stack operation is a push.
    vmcs.write(encoding::HOST_RSP, ptr_to_num(&host_stack[HOST_STACK_SIZE]));
    vmcs.write(encoding::HOST_RIP, ptr_to_num(exit_handler_low_level));

    // Enable the minimum pin-based execution controls.
    //
    // If bit 55 in MSR IA32_VMX_BASIC is set, the minimum can be read from
    // IA32_VMX_TRUE_PINBASED_CTLS (Intel SDM, Vol. 3, A.3.1 - Pin-Based VM-Execution Controls).
    // Otherwise, IA32_VMX_PINBASED_CTLS has to be consulted.
    //
    // In both MSRs, bits 0-31 report on the allowed 0 settings.
    // If bit X in this part of the MSR is set, it has to be set in the
    // pin-based execution controls.
    // The lower 32 bit of the MSR therefore define the minimum.
    if (rdmsr(x86::IA32_VMX_BASIC) & to_underlying(x86::vmx_basic_constants::OVERRIDE_DEFAULT_ONE_CLASS)) {
        vmcs.write(encoding::PIN_BASED_EXEC_CTRL, static_cast<uint32_t>(rdmsr(x86::IA32_VMX_TRUE_PINBASED_CTLS)));
    }
    else {
        vmcs.write(encoding::PIN_BASED_EXEC_CTRL, static_cast<uint32_t>(rdmsr(x86::IA32_VMX_PINBASED_CTLS)));
    }

    // configure primary execution controls
    uint32_t primary_exec{ static_cast<uint32_t>(vmcs.read(encoding::PRIMARY_EXEC_CTRL)) };

    // set default 1 execution controls (musts)
    primary_exec |= to_underlying(x86::vmx_primary_exc_ctls_constants::DEFAULT_ONE);

    // Enable use of MSR bitmaps (table 24-6). Otherwise MSR access triggers exit.
    // MSR bitmap configuration follows later.
    primary_exec |= to_underlying(x86::vmx_primary_exc_ctls_constants::USE_MSR_BITMAPS);

    // Disable IO exits.
    // don't use I/O bitmaps -> unconditional I/O exiting control is used
    primary_exec &= ~to_underlying(x86::vmx_primary_exc_ctls_constants::USE_IO_BITMAPS);
    // disable unconditional I/O exiting
    primary_exec &= ~to_underlying(x86::vmx_primary_exc_ctls_constants::UNCONDITIONAL_IO_EXITING);

    vmcs.write(encoding::PRIMARY_EXEC_CTRL, primary_exec);

    // All the exit control fields are already set to 0.
    // We configure an MSR bitmap which is also filled with 0s to disable MSR exits.
    vmcs.write(encoding::MSR_BITMAP_A, ptr_to_num(&msr_exit_bitmaps));

    vmcs.write(encoding::CR4_GUEST_HOST_MASK, ~0ull);
}

void tinivisor::disable_secondary_execution_controls()
{
    uint32_t primary_exec{ static_cast<uint32_t>(vmcs.read(encoding::PRIMARY_EXEC_CTRL)) };
    primary_exec &= ~to_underlying(x86::vmx_primary_exc_ctls_constants::ACTIVATE_SEC_EXEC_CTRLS);
    vmcs.write(encoding::PRIMARY_EXEC_CTRL, primary_exec);
}

void tinivisor::configure_64_bit_host()
{
    uint32_t exit{ static_cast<uint32_t>(vmcs.read(encoding::VM_EXI_CTRL)) };
    exit |= to_underlying(x86::vmx_exit_ctrls_constants::DEFAULT_ONE);
    // exits are handled by a 64 bit host (Vol 3, 24.7.1)
    exit |= to_underlying(x86::vmx_exit_ctrls_constants::HOST_ADDR_SPACE_SIZE);
    vmcs.write(encoding::VM_EXI_CTRL, exit);
}
void tinivisor::configure_64_bit_guest()
{
    uint32_t entry{ static_cast<uint32_t>(vmcs.read(encoding::VM_ENT_CTRL)) };
    entry |= to_underlying(x86::vmx_entry_ctrls_constants::DEFAULT_ONE);
    // use 64 bit guests (Vol. 3, 24.8.1)
    entry |= to_underlying(x86::vmx_entry_ctrls_constants::IA32_MODE_GUEST);
    vmcs.write(encoding::VM_ENT_CTRL, entry);
}

void tinivisor::exit_handler(guest_regs& regs)
{
    vmcs::vmcs_t* vmcs{ reinterpret_cast<vmcs::vmcs_t*>(vmptrst()) };
    assert(vmcs != nullptr);

    x86::vmx_exit_reason exit_reason{ static_cast<uint32_t>(vmcs->read(encoding::EXI_REASON)) };
    uint64_t exit_qual{ vmcs->read(encoding::EXI_QUAL) };

    handlers.at(to_underlying(exit_reason))(*vmcs, regs, exit_reason, exit_qual);
}
