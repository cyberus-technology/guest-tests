/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <array>
#include <assert.h>
#include <functional>

#include <toyos/x86/vmcs.hpp>

/**
 * \brief Tiny hypervisor helper.
 *
 * Tinivisor is a helper for creating minimalistic VMX setups.
 * It can be used to transfer the current control flow to a virtual machine.
 * This virtual machine continues to use the execution environment, e.g. address space, prepared so far.
 * One can configure the host's reactions to guest VM exits. As an example, the return value value of a guest's cpuid
 * call can be controlled.
 * The virtual machine can be turned off with a special request.
 * Upon such a request, current control flow is transferred from the virtual machine and VMX is turned off.
 *
 * Tinivisor expects to be used in 64 bit mode. It also expects segmentation to adhere to
 * Intel SDM, Vol. 3, 26.2.3 - Checks on Host Segment and Descriptor-Table Registers and 26.3.1.2 - Checks on Guest
 * Segment Registers and does not check for violations.
 *
 * Note that only a single instance of tinivisor is expected to exist.
 */
class tinivisor
{
 public:
    void reset();

    /**
     * \brief Guest register state.
     *
     * This state can be used to modify the guest register values during exit handling.
     * Other registers, like rsp, are accessible via VMCS.
     * The order of elements has to adhere to vmxexit.S.
     */
    struct guest_regs
    {
        uint64_t r15, r14, r13, r12;
        uint64_t r11, r10, r9, r8;
        uint64_t rdi, rsi, rbp;
        uint64_t rdx, rcx, rbx, rax;
    };

    /**
     * \brief Switch to VMX non-root mode.
     *
     * Enable VMX, prepare a VMCS, and switch to the guest.
     * The virtual machine is configured with minimum settings:
     *  - the current address space (including stack) are used
     *  - current segmentation settings are cloned
     *  - VM exits are disabled for MSR and I/O access
     * Other VM exits are handled on a dedicated stack and by a default handler which dumps information about
     * the exit and then stops execution.
     * A different handler can be configured using register_handler.
     *
     * This functions returns in VMX non-root mode.
     */
    void start();

    /**
     * \brief Leave VMX mode and disable VMX.
     *
     * When stopping the virtual machine, control is transferred back from the guest.
     * Internally, this method triggers a VM exit which switches to the exit handler. As part of handling the exit,
     * the guest's architectural state is saved. Still in VMX root mode, guest register values are restored and
     * it is switched from the exit handler stack to the stack used by the guest when calling tinivisor::stop.
     *
     * Hence, the virtual machine's shut down is nearly transparent to the caller of this function, because local and
     * global variables can be used. Of course, the shutdown is noticeable because registered exit handlers will
     * not be called anymore
     */
    __NOINLINE__ void stop();

    /**
     * \brief Signature of a VM exit handler.
     *
     * It has access to the VMCS and register state of the guest.
     * The other two parameters are exit reason and exit qualification.
     */
    using handler_func = std::function<void(vmcs::vmcs_t&, guest_regs&, x86::vmx_exit_reason, uint64_t)>;

    /**
     * \brief Register a VM exit handler.
     *
     * \param reason number of the exit to be handled
     * \param handler pointer to handler function
     *
     * Note that your handler might have to adjust the guest's instruction pointer.
     * The VMCALL exit is used to shut down tinivisor and registering a handler for VMCALL is forbidden.
     */
    static void register_handler(x86::vmx_exit_reason reason, const handler_func& handler)
    {
        assert(reason != x86::vmx_exit_reason::VMCALL);
        handlers.at(to_underlying(reason)) = handler;
    }

 private:
    /**
     * \brief Get VMCS revision identifier used by the processor.
     *
     * \return revision identifier
     */
    uint32_t get_vmcs_revision_id() const;

    /**
     * \brief Set lock bit in feature control MSR.
     *
     * Calling VMXON with unlocked feature controls causes a GP (Intel SDM, Vol. 3, 23.7 - Enabling and Entering VMX
     * Operation).
     * Before reading feature controls, make sure that this MSR is locked so
     * that the features won't change.
     */
    void lock_feature_control_msr();

    /**
     * \brief Check whether VMX is supported and VMXON can be called.
     *
     * \return VMX supported
     */
    bool is_vmx_supported() const;

    /**
     * \brief Update control registers for VMX mode.
     *
     * The platform defines bits in CR0 and CR4 which have to be set/clear in VMX mode.
     * These settings are read from MSRs and applied.
     */
    void apply_vmx_enforced_control_register_bits();

    /**
     * \brief Clone current segmentation settings to guest and host state.
     *
     * The current settings should be used in both VMX root and non-root mode.
     * But in addition to cloning, they have to be sanitized to adhere to
     * the hardware checks performed during load of guest and host state.
     */
    void clone_segments();

#define CLONE_TO_GUEST_SEGMENT(sel)                                                                           \
    const vmcs::cached_segment_descriptor& clone_to_guest_##sel(vmcs::vmcs_t& vmcs_,                          \
                                                                const vmcs::cached_segment_descriptor& descr) \
    {                                                                                                         \
        using vmcs_enc = x86::vmcs_encoding;                                                                  \
        vmcs_.write(vmcs_enc::GUEST_SEL_##sel, descr.selector);                                               \
        vmcs_.write(vmcs_enc::GUEST_BASE_##sel, descr.base);                                                  \
        vmcs_.write(vmcs_enc::GUEST_LIMIT_##sel, descr.limit);                                                \
        vmcs_.write(vmcs_enc::GUEST_AR_##sel, descr.ar.get_value());                                          \
        return descr;                                                                                         \
    }

    CLONE_TO_GUEST_SEGMENT(CS);
    CLONE_TO_GUEST_SEGMENT(SS);
    CLONE_TO_GUEST_SEGMENT(DS);
    CLONE_TO_GUEST_SEGMENT(ES);
    CLONE_TO_GUEST_SEGMENT(FS);
    CLONE_TO_GUEST_SEGMENT(GS);
    CLONE_TO_GUEST_SEGMENT(LDTR);
    CLONE_TO_GUEST_SEGMENT(TR);

#define CLONE_TO_GUEST_SEGMENT_BASIC(sel)                                                                        \
    const x86::descriptor_ptr& clone_to_guest_basic_##sel(vmcs::vmcs_t& vmcs_, const x86::descriptor_ptr& descr) \
    {                                                                                                            \
        using vmcs_enc = x86::vmcs_encoding;                                                                     \
        vmcs_.write(vmcs_enc::GUEST_BASE_##sel, descr.base);                                                     \
        vmcs_.write(vmcs_enc::GUEST_LIMIT_##sel, descr.limit);                                                   \
        return descr;                                                                                            \
    }

    CLONE_TO_GUEST_SEGMENT_BASIC(GDTR);
    CLONE_TO_GUEST_SEGMENT_BASIC(IDTR);

    /**
     * \brief Clone current values of cr0, cr3, and cr4 to guest and host state.
     *
     * Make sure to call apply_vmx_enforced_control_register_bits() first.
     */
    void clone_control_registers();

    /**
     * \brief Prepare VM exit handling.
     *
     * Configures when to cause exits and how they are handled.
     */
    void configure_exits();

    /**
     * \brief Disable secondary execution controls.
     *
     * When secondary execution controls are disabled, the processor treats them as 0 (Intel SDM, Vol. 3, 24.6.2 -
     * Processor-Based VM-Execution Controls).
     */
    void disable_secondary_execution_controls();

    void configure_64_bit_guest();
    void configure_64_bit_host();

    /**
     * \brief High level entry point for VM exit handling.
     *
     * \param regs guest register contents
     */
    static void exit_handler(guest_regs& regs) asm("vmx_handler");

    /**
     * \brief Default exit handler. Prints information about the exit and halts.
     */
    static void default_exit_handler(vmcs::vmcs_t& vmcs, guest_regs& regs, x86::vmx_exit_reason reason, uint64_t exit_qual);

    /**
     * \brief Memory region used by vmxon.
     *
     * Before providing this memory region to vmxon, a VMCS revision ID has to be written to its beginning.
     */
    union vmxon_region
    {
        uint8_t raw[PAGE_SIZE];  // only used by hardware
        uint32_t rev_id;
    };

    alignas(PAGE_SIZE) vmxon_region vmxon_page;

    // VMCS for the virtual machine managed by tinivisor
    vmcs::vmcs_t vmcs;

    // TODO: find a way to detect stack overflows
    static constexpr size_t HOST_STACK_PAGES{ 1 };
    static constexpr size_t HOST_STACK_SIZE{ HOST_STACK_PAGES * PAGE_SIZE };
    alignas(PAGE_SIZE) uint8_t host_stack[HOST_STACK_SIZE];

    // space for 4 MSR bitmaps, each 1 KB in size (Intel SDM, Vol. 3, 24.6.9 - MSR-Bitmap Address)
    alignas(PAGE_SIZE) uint8_t msr_exit_bitmaps[PAGE_SIZE];

    // VM exit handler
    using handler_container_t = std::array<handler_func, 256>;
    static handler_container_t handlers;
};
