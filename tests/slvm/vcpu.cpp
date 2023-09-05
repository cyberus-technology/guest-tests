/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <cstdint>

#include <arch.hpp>
#include <asm_code_helper.hpp>
#include <baretest/baretest.hpp>
#include <cbl/lock_free_queue.hpp>
#include <cpuid.hpp>
#include <hedron/utcb_layout.hpp>
#include <irq_handler.hpp>
#include <lapic_enabler.hpp>
#include <lapic_lvt_guard.hpp>
#include <lapic_test_tools.hpp>
#include <slvm/api.h>
#include <slvm_info_page.hpp>
#include <speculation.hpp>
#include <trace.hpp>
#include <vmx.hpp>
#include <x86asm.hpp>
#include <x86defs.hpp>

#include <array>
#include <memory>

using namespace lapic_test_tools;
using baretest::expectation;

namespace
{
const uint64_t initial_rip {0x1000};
const uint64_t page_table_gpa {0x10000000};

/// Returns the vCPU state for 16-Bit Real Mode.
std::unique_ptr<hedron::utcb_layout> mode16_state()
{
    // Avoid allocating a large structure on the stack to prevent stack overflow.
    auto utcb {std::make_unique<hedron::utcb_layout>()};

    memset(utcb.get(), 0, sizeof(*utcb));

    utcb->rflags = x86::FLAGS_MBS;
    utcb->ip = initial_rip;

    utcb->xcr0 = x86::XCR0_FPU;

    utcb->cs = hedron::segment_descriptor({.sel = 0x0000, .ar = 0x93, .limit = 0xFFFF, .base = 0x0});
    utcb->ds = utcb->es = utcb->fs = utcb->gs = utcb->ss =
        hedron::segment_descriptor({.sel = 0x0000, .ar = 0x93, .limit = 0xFFFF, .base = 0x0});

    utcb->tr = hedron::segment_descriptor({.sel = 0, .ar = 0x8b, .limit = 0xFFFF, .base = 0});
    utcb->gdtr = utcb->idtr = utcb->ldtr =
        hedron::segment_descriptor({.sel = 0, .ar = 0x82, .limit = 0xFFFF, .base = 0});

    utcb->dr7 = 0x400;

    utcb->mtd.set(hedron::mtd_bits::DEFAULT);

    return utcb;
}

/// Returns the vCPU state for 64-Bit Long Mode.
std::unique_ptr<hedron::utcb_layout> mode64_state()
{
    // Avoid allocating a large structure on the stack to prevent stack overflow.
    auto utcb {std::make_unique<hedron::utcb_layout>()};

    memset(utcb.get(), 0, sizeof(*utcb));

    utcb->rflags = x86::FLAGS_MBS;
    utcb->ip = initial_rip;

    utcb->xcr0 = x86::XCR0_FPU;

    utcb->cs = hedron::segment_descriptor({.sel = 0x8, .ar = 0xa9b, .limit = 0xFFFFFFFF, .base = 0});
    utcb->ds = utcb->es = utcb->fs = utcb->gs =
        hedron::segment_descriptor({.sel = 0x0, .ar = 0xa93, .limit = 0xFFFFFFFF, .base = 0});
    utcb->ss = hedron::segment_descriptor({.sel = 0x10, .ar = 0xa93, .limit = 0xFFFFFFFF, .base = 0});

    utcb->tr = hedron::segment_descriptor({.sel = 0x30, .ar = 0x8b, .limit = 0x68, .base = 0});
    utcb->gdtr = utcb->idtr = utcb->ldtr =
        hedron::segment_descriptor({.sel = 0, .ar = 0x82, .limit = 0xFFFF, .base = 0});

    utcb->dr7 = 0x400;

    utcb->efer = x86::EFER_LME | x86::EFER_LMA;
    utcb->cr0 = math::mask_from(x86::cr0::PE, x86::cr0::PG, x86::cr0::ET);
    utcb->cr4 = math::mask_from(x86::cr4::PAE);

    utcb->mtd.set(hedron::mtd_bits::DEFAULT);

    return utcb;
}

/// Enable XSAVE (in this VM, not the SLVM VMs).
void enable_xsave()
{
    set_cr0(get_cr0() & ~math::mask_from(x86::cr0::EM));
    set_cr4(get_cr4() | math::mask_from(x86::cr4::OSFXSR, x86::cr4::OSXSAVE));
    set_xcr(x86::XCR0_FPU | x86::XCR0_SSE);
}

/// Modes in which the VM can be created.
enum class vm_mode
{
    MODE16, // 16-Bit Real Mode
    MODE64, // 64-Bit Long Mode
};

std::unique_ptr<hedron::utcb_layout> initial_state_for(vm_mode mode)
{
    switch (mode) {
    case vm_mode::MODE16: return mode16_state();
    case vm_mode::MODE64: return mode64_state();
    default: PANIC("Unknown mode");
    }
}

// The list of exit reasons that we just ignore unless otherwise specified.
const std::vector<uint32_t>& default_ignored_exits()
{
    static const std::vector<uint32_t> exits_to_ignore = {VMX_PT_POKED, VMX_PT_PREEMPTION_TIMER};
    return exits_to_ignore;
}

/// A VM with a single vCPU implemented using the SLVM API.
struct slvm_vm
{
    static_assert(SLVMAPI_VMCALL_VERSION == 3, "API Version mismatch, please update this abstraction to the new API!");

    const vm_mode mode;

    alignas(PAGE_SIZE) hedron::utcb_layout vcpu_state {*initial_state_for(mode)};
    alignas(PAGE_SIZE) std::array<uint8_t, PAGE_SIZE> xsave_area {};

    /// One page of memory that we use in prepare_to_execute().
    alignas(PAGE_SIZE) std::array<uint8_t, PAGE_SIZE> guest_memory;

    /// The number of page table pages we need.
    static constexpr size_t guest_page_table_pages {3};

    /// Backing store for memory that is used as page tables.
    ///
    /// Not used unless enable_long_mode() is called.
    alignas(PAGE_SIZE) std::array<uint64_t, guest_page_table_pages * PAGE_SIZE / sizeof(uint64_t)> guest_page_tables {};

    /// Identifier for the VM in the SLVM API.
    slvm_vm_id_t vm_id;

    /// Describes what FPU state we want saved and restored from xsave_area.
    uint64_t xstate_components {x86::XCR0_FPU | x86::XCR0_SSE};

    /// Set up paging for 64-bit execution.
    ///
    /// This function creates an identity-mapped page table that covers 2MB of memory starting from GVA 0.
    ///
    /// \param guest_pt_dst
    ///        A guest physical address that will be used to map page tables. This needs space for
    ///        guest_page_table_pages pages.
    void enable_long_mode_paging(uint64_t guest_pt_dst)
    {
        const auto PAGE_P {static_cast<uint64_t>(1) << 0};
        const auto PAGE_W {static_cast<uint64_t>(1) << 1};
        const auto PAGE_L {static_cast<uint64_t>(1) << 7}; // L = Large Page

        assert(math::is_aligned(guest_pt_dst, PAGE_BITS));

        guest_page_tables.fill(0);
        guest_page_tables[0] = (guest_pt_dst + PAGE_SIZE) | PAGE_W | PAGE_P;
        guest_page_tables[512] = (guest_pt_dst + 2 * PAGE_SIZE) | PAGE_W | PAGE_P;
        guest_page_tables[1024] = PAGE_L | PAGE_W | PAGE_P;

        assert(sizeof(guest_page_tables) / PAGE_SIZE == guest_page_table_pages);

        map_memory(addr2pn(reinterpret_cast<uintptr_t>(guest_page_tables.data())), guest_pt_dst >> PAGE_BITS,
                   guest_page_table_pages, SLVMAPI_PROT_READ | SLVMAPI_PROT_WRITE);

        vcpu_state.cr3 = guest_pt_dst;
        vcpu_state.mtd.set(hedron::mtd_bits::CR);
    }

    int run_vcpu()
    {
        const uint64_t state_gfn {addr2pn(reinterpret_cast<uintptr_t>(&vcpu_state))};
        const uint64_t xsave_gfn {addr2pn(reinterpret_cast<uintptr_t>(&xsave_area))};

        slvm_status_t status {slvm_vmcall_run_vcpu_v4(vm_id, state_gfn, xsave_gfn, false).status};
        PANIC_UNLESS(status >= 0, "Failed to run vCPU");

        return status;
    }

    /// Enter guest execution just like run_vcpu() but ignore unwanted exits.
    /// Per default, we disable POKED and PREEMPTION_TIMER exits which can occur spuriously.
    int run_vcpu_ignore_exits(const std::vector<uint32_t>& exits_to_ignore = default_ignored_exits())
    {
        slvm_status_t status;

        do {
            status = run_vcpu();
        } while (std::find(exits_to_ignore.begin(), exits_to_ignore.end(), status) != exits_to_ignore.end());

        return status;
    }

    void map_memory(uint64_t src_fn, uint64_t dst_gfn, size_t pages, uint32_t prot)
    {
        slvm_status_t status {slvm_vmcall_map_memory(vm_id, src_fn, dst_gfn, pages, prot)};
        PANIC_UNLESS(status == SLVMAPI_VMCALL_SUCCESS, "Failed to map memory");
    }

    void unmap_memory(uint64_t dst_gfn, size_t pages)
    {
        slvm_status_t status {slvm_vmcall_unmap_memory(vm_id, dst_gfn, pages)};
        PANIC_UNLESS(status == SLVMAPI_VMCALL_SUCCESS, "Failed to unmap memory");
    }

    bool register_write_page(void* write_page, uint64_t max_timeout_ms = 5, uint64_t max_outstanding_requests = 64,
                             uint64_t lfq_api_version = LFQ_API_VERSION) const
    {
        uintptr_t ptr {reinterpret_cast<uintptr_t>(write_page)};

        slvm_status_t status {slvm_vmcall_register_write_page(vm_id, addr2pn(ptr), max_timeout_ms,
                                                              max_outstanding_requests, lfq_api_version)};

        return status == SLVMAPI_VMCALL_SUCCESS;
    }

    bool register_mmio_region(uint64_t mmio_gpa, uint64_t mmio_size) const
    {
        slvm_status_t status {slvm_vmcall_register_mmio_region(vm_id, mmio_gpa, mmio_size)};
        return status == SLVMAPI_VMCALL_SUCCESS;
    }

    bool unregister_mmio_region(uint64_t mmio_gpa, uint64_t mmio_size) const
    {
        slvm_status_t status {slvm_vmcall_unregister_mmio_region(vm_id, mmio_gpa, mmio_size)};
        return status == SLVMAPI_VMCALL_SUCCESS;
    }

    /// Modify the VM to execute the given code on the next VM entry.
    void prepare_to_execute(const std::vector<uint8_t>& code)
    {
        assert(math::is_aligned(initial_rip, PAGE_BITS));

        vcpu_state.ip = initial_rip;
        vcpu_state.mtd.set(hedron::mtd_bits::EIP);

        std::copy(code.begin(), code.end(), guest_memory.begin());

        map_memory(addr2pn(reinterpret_cast<uintptr_t>(guest_memory.data())), initial_rip >> PAGE_BITS, 1U,
                   SLVMAPI_PROT_READ | SLVMAPI_PROT_WRITE | SLVMAPI_PROT_EXEC);
    }

    /// Enable XSAVE in the VM.
    void enable_xsave()
    {
        vcpu_state.cr4 |= static_cast<uint64_t>(x86::cr4::OSFXSR);
        vcpu_state.mtd.set(hedron::mtd_bits::CR);
    }

    /// Set the low 64-bits of a SSE register.
    void set_xmm_lo(unsigned no, uint64_t lo)
    {
        assert(no < 16);

        // See the Intel SDM Vol. 1 13.4 for the layout of the XSAVE area.
        memcpy(xsave_area.data() + 160 + no * 16, &lo, sizeof(lo));
    }

    /// Get the low 64-bits of a SSE register.
    uint64_t get_xmm_lo(unsigned no) const
    {
        assert(no < 16);

        uint64_t lo;

        // See the Intel SDM Vol. 1 13.4 for the layout of the XSAVE area.
        memcpy(&lo, xsave_area.data() + 160 + no * 16, sizeof(lo));

        return lo;
    }

    uint8_t get_tpr() const { return static_cast<uint8_t>(vcpu_state.header.tls) >> 4; }

    void set_tpr(uint8_t tpr) { vcpu_state.header.tls = tpr << 4; }

    // This class cannot be copied, because we would double destroy the VM.
    slvm_vm(const slvm_vm& other) = delete;
    slvm_vm& operator=(const slvm_vm& other) = delete;

    slvm_vm() = delete;

    explicit slvm_vm(vm_mode mode_) : mode(mode_)
    {
        slvm_status_t status {slvm_vmcall_create_vm()};
        PANIC_ON(status < 0, "Failed to create VM");

        vm_id = status;

        if (mode == vm_mode::MODE64) {
            enable_long_mode_paging(page_table_gpa);
        }
    }

    ~slvm_vm()
    {
        slvm_status_t status {slvm_vmcall_destroy_vm(vm_id)};
        PANIC_UNLESS(status == SLVMAPI_VMCALL_SUCCESS, "Failed to destroy VM");
    }
};

/// A helper class to schedule interrupts when requested.
///
/// The source of the interrupts is not important. The intention is to use this class to test whether slave VM execution
/// is reliably interrupted, when a host IRQ needs to be delivered.
class irq_generator
{
    static constexpr int UNUSED {-1};
    static inline volatile int lapic_irq_count {UNUSED};

    static void counting_irq_handler([[maybe_unused]] intr_regs* regs)
    {
        lapic_irq_count++;
        send_eoi();
    }

    irq_handler::guard handler_guard {&counting_irq_handler};
    lapic_enabler lapic_enabler_ {}; // The LAPIC has to be enabled before configuring an LVT.
    lvt_guard lvt_entry_guard {lvt_entry::TIMER, MAX_VECTOR, lvt_modes::ONESHOT};

public:
    irq_generator(const irq_generator&) = delete;
    irq_generator& operator=(const irq_generator&) = delete;

    /// Schedule an interrupt to be delivered "soon".
    void schedule_irq() { write_to_register(LAPIC_INIT_COUNT, 0xFFFFFFFU); }

    irq_generator()
    {
        // Prevent nested use of this class, because it will not work.
        assert(lapic_irq_count == UNUSED);
        lapic_irq_count = 0;

        mask_pic();

        write_divide_conf(1);
        enable_interrupts();
    }

    ~irq_generator()
    {
        disable_interrupts();
        lapic_irq_count = UNUSED;
    }
};

} // namespace

// A simple HLT instructions is commonly used, so define it centrally.
ASM_CODE(guest_code_hlt, "hlt;");

TEST_CASE(can_run_guest_instructions)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE16)};

    // With no mappings, we can only get an EPT violation at the initial RIP.
    BARETEST_ASSERT(vm->run_vcpu_ignore_exits() == VMX_PT_EPT_VIOLATION);
    BARETEST_ASSERT(vm->vcpu_state.qual[1] == initial_rip);

    // If we now map code, we see this code being executed.
    vm->prepare_to_execute(ASM_BUFFER(guest_code_hlt));
    BARETEST_ASSERT(vm->run_vcpu_ignore_exits() == VMX_PT_HLT);
}

TEST_CASE(can_unmap_memory)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE16)};

    // If we map code ...
    vm->prepare_to_execute(ASM_BUFFER(guest_code_hlt));

    // ... and then unmap the code page from the EPT ...
    vm->unmap_memory((vm->vcpu_state.ip + vm->vcpu_state.cs.base) >> PAGE_BITS, 1);

    // We don't see the code executed.
    BARETEST_ASSERT(vm->run_vcpu_ignore_exits() == VMX_PT_EPT_VIOLATION);
}

TEST_CASE(interrupts_interrupt_guest_execution)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE16)};
    const size_t retries {5};

    // Execute an endless loop in the guest, so we can only exit on host IRQs
    ASM_CODE(guest_code_jmp0, "0: jmp 0b;");
    vm->prepare_to_execute(ASM_BUFFER(guest_code_jmp0));

    {
        irq_generator irq_gen;

        for (size_t i = 0; i < retries; i++) {
            irq_gen.schedule_irq();

            const auto exit_reason {vm->run_vcpu_ignore_exits({VMX_PT_PREEMPTION_TIMER})};
            BARETEST_ASSERT(exit_reason == VMX_PT_POKED);
        }
    }
}

TEST_CASE(fpu_is_context_switched)
{
    ASM_CODE(guest_code_eax_to_xmm0, "movd %eax, %xmm0; hlt;");
    ASM_CODE(guest_code_xmm0_to_eax, "movd %xmm0, %eax; hlt;");

    enable_xsave();

    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE16)};
    const uint32_t magic1 {0xCAFED00D};
    const uint32_t magic2 {0xDECAFBAD};
    int exit_reason;

    vm->enable_xsave();

    // Check whether we get the FPU state from the guest out. To do this we pass a value in a GPR, the VM puts it into
    // the FPU state and we check whether it actually ends up there.

    vm->vcpu_state.ax = magic1;
    vm->vcpu_state.mtd.set(hedron::mtd_bits::ACDB);

    vm->prepare_to_execute(ASM_BUFFER(guest_code_eax_to_xmm0));
    exit_reason = vm->run_vcpu_ignore_exits(default_ignored_exits());

    BARETEST_ASSERT(exit_reason == VMX_PT_HLT);
    BARETEST_ASSERT(vm->get_xmm_lo(0) == magic1);

    // Now we check whether we can hand in a value. This works in reverse to the test above.

    vm->set_xmm_lo(0, magic2);

    vm->prepare_to_execute(ASM_BUFFER(guest_code_xmm0_to_eax));
    exit_reason = vm->run_vcpu_ignore_exits(default_ignored_exits());

    BARETEST_ASSERT(exit_reason == VMX_PT_HLT);
    BARETEST_ASSERT(vm->vcpu_state.ax == magic2);
}

TEST_CASE(tpr_threshold)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    vm->vcpu_state.ctrl[0] = 0;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_TPR_SHADOW;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_SECONDARY;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_HLT;
    vm->vcpu_state.ctrl[0] &= ~VMX_CTRL0_CR8_STORE;

    vm->vcpu_state.ctrl[1] = 0;
    vm->vcpu_state.ctrl[1] |= VMX_CTRL1_URG;
    vm->vcpu_state.ctrl[1] |= VMX_CTRL1_EPT;
    vm->vcpu_state.mtd.set(hedron::mtd_bits::CTRL);

    // Do we get a 'TPR below threshold' exit when we change the TPR from old_tpr to new_tpr given a specific threshold.
    ASM_CODE(guest_code_load_tpr_from_rax, "mov %rax, %cr8; hlt;");
    auto does_tpr_below_threshold_exit = [&vm](uint8_t threshold, uint8_t old_tpr, uint8_t new_tpr) -> bool {
        vm->vcpu_state.tpr_threshold = threshold;
        vm->vcpu_state.mtd.set(hedron::mtd_bits::TPR);

        vm->set_tpr(old_tpr);

        vm->vcpu_state.ax = new_tpr;
        vm->vcpu_state.mtd.set(hedron::mtd_bits::ACDB);

        vm->prepare_to_execute(ASM_BUFFER(guest_code_load_tpr_from_rax));

        auto exit {vm->run_vcpu_ignore_exits()};
        assert(exit == VMX_PT_TPR_THRESHOLD or exit == VMX_PT_HLT);

        return exit == VMX_PT_TPR_THRESHOLD;
    };

    BARETEST_ASSERT(not does_tpr_below_threshold_exit(0x8, 0xf, 0x9));
    BARETEST_ASSERT(does_tpr_below_threshold_exit(0x8, 0xf, 0x7));
}

TEST_CASE_CONDITIONAL(spec_ctrl_ctx_switch, ibrs_supported())
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    const uint64_t spec_ctrl_host {x86::SPEC_CTRL_IBRS};

    // Allow MSR accesses in the guest
    vm->vcpu_state.ctrl[0] = 0;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_SECONDARY;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_HLT;
    vm->vcpu_state.ctrl[0] |= VMX_CTRL0_MSR_BITMAP;
    vm->vcpu_state.ctrl[0] &= ~VMX_CTRL0_CR8_STORE;

    vm->vcpu_state.ctrl[1] = 0;
    vm->vcpu_state.ctrl[1] |= VMX_CTRL1_URG;
    vm->vcpu_state.ctrl[1] |= VMX_CTRL1_EPT;
    vm->vcpu_state.mtd.set(hedron::mtd_bits::CTRL);

    // Turn on IBRS in the host.
    wrmsr(x86::IA32_SPEC_CTRL, spec_ctrl_host);

    // IA32_SPEC_CTRL = 0x000000048
    ASM_CODE(guest_code_load_spec_ctrl, "mov $0x000000048, %ecx; rdmsr; hlt;");
    ASM_CODE(guest_code_set_spec_ctrl, "mov $0x000000048, %ecx; xor %eax, %eax; xor %edx, %edx; wrmsr; hlt;");

    // Test 1: Host value must not leak into guest.
    vm->prepare_to_execute(ASM_BUFFER(guest_code_load_spec_ctrl));
    auto exit {vm->run_vcpu_ignore_exits()};
    BARETEST_ASSERT(exit == VMX_PT_HLT);

    BARETEST_ASSERT(vm->vcpu_state.ax == 0);
    BARETEST_ASSERT(vm->vcpu_state.dx == 0);
    BARETEST_ASSERT(rdmsr(x86::IA32_SPEC_CTRL) == spec_ctrl_host);

    // Test 2: Guest value must not leak into host.
    vm->prepare_to_execute(ASM_BUFFER(guest_code_set_spec_ctrl));
    exit = vm->run_vcpu_ignore_exits();

    BARETEST_ASSERT(exit == VMX_PT_HLT);
    BARETEST_ASSERT(rdmsr(x86::IA32_SPEC_CTRL) == spec_ctrl_host);

    // Cleanup
    wrmsr(x86::IA32_SPEC_CTRL, 0);
}

// This test depends on behavior that can vary between CPUs.
// Sometimes there is no reliable way to trigger the VM entry failure, in which
// case we can only skip the test.
// One example for this is the i7-3610QE found in the SINA WS/H Client IIIa.
namespace
{
bool vmentry_failure_unreliable()
{
    const auto info {x86::get_cpu_info()};
    return info == x86::cpu_info {0x6, 0x3a, 0x9};
}
} // namespace
TEST_CASE_CONDITIONAL(vmentry_failure_early, not vmentry_failure_unreliable())
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    // By injecting a malformed exception (vector >= 32), we trigger an early VM entry failure. This failure will cause
    // the VMRESUME instruction to fail and the CPU does not load host state. This means the kernel has to take a
    // different path to deal with this error scenario.
    auto& inj_info {vm->vcpu_state.inj_info};

    inj_info.clear();
    inj_info.type(to_underlying(x86::intr_type::HW_EXC));
    inj_info.vector(32);
    inj_info.error(false);
    inj_info.valid(true);

    vm->vcpu_state.mtd.set(hedron::mtd_bits::INJ);

    BARETEST_VERIFY(expectation<uint32_t>(VMX_PT_VMENTRY_FAIL) == vm->run_vcpu_ignore_exits());
}

TEST_CASE(vmentry_failure_late)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    // Setting CR3 to an invalid value (Intel SDM Vol 3 26.3.1.1) causes the CPU to load host state normally (i.e. we
    // come via the normal exit path), but the VM entry failure in the exit reason VMCS field is set.
    //
    // This is the "happy" VM entry failure, because it takes the usual path in the kernel.
    vm->vcpu_state.cr3 = ~0ull;

    BARETEST_VERIFY(expectation<uint32_t>(VMX_PT_INVALID_GUEST_STATE) == vm->run_vcpu_ignore_exits());
}

TEST_CASE(invalid_xcr0_is_vmentry_failure)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    vm->vcpu_state.xcr0 = 0;

    // Hedron has to manually context switch XCR0. It needs to deal with invalid XCR0 values gracefully.
    BARETEST_VERIFY(expectation<uint32_t>(VMX_PT_INVALID_GUEST_STATE) == vm->run_vcpu_ignore_exits());
}

namespace
{

bool operator==(const posted_writes_descriptor& l, const posted_writes_descriptor& r)
{
    return l.addr == r.addr and l.size == r.size and l.buf == r.buf;
}

template <size_t SIZE>
struct aligned_memory
{
    alignas(SIZE) std::array<uint8_t, SIZE> data;
};

struct consumer_queue
{
    using queue = cbl::lock_free_queue_consumer<posted_writes_descriptor, POSTED_WRITES_QUEUE_SIZE>;

    consumer_queue(const slvm_vm& vm) : mem(std::make_unique<aligned_memory<PAGE_SIZE>>())
    {
        BARETEST_ASSERT(vm.register_write_page(mem->data.data()));

        queue::queue_storage* storage {reinterpret_cast<queue::queue_storage*>(mem->data.data())};
        cons_q = queue::create(*storage);
        BARETEST_ASSERT(cons_q);
    }

    std::unique_ptr<queue> cons_q;
    std::unique_ptr<aligned_memory<PAGE_SIZE>> mem;
};

} // anonymous namespace

static void check_posted_writes(slvm_vm& vm)
{
    consumer_queue q {vm};

    const uintptr_t dummy_addr {0x10000};
    BARETEST_ASSERT(vm.register_mmio_region(dummy_addr, PAGE_SIZE));

    ASM_CODE(guest_code_mov_hlt, "movl $0xc0fed00d, 0x10000; hlt;");

    vm.prepare_to_execute(ASM_BUFFER(guest_code_mov_hlt));
    BARETEST_ASSERT(vm.run_vcpu_ignore_exits() == VMX_PT_HLT);

    const auto expected {posted_writes_descriptor {.addr = dummy_addr, .size = 4ul, .buf = 0xc0fed00d}};
    BARETEST_ASSERT(q.cons_q->front() == expected);
}

TEST_CASE(posted_writes_api_fails_on_wrong_usage)
{
    auto mem {std::make_unique<aligned_memory<PAGE_SIZE>>()};
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    const uintptr_t dummy_addr {0x10000};
    const uintptr_t default_max_timeout {5};
    const uintptr_t default_max_requests {64};

    const size_t aligned_address_4k {0x2000};
    const size_t power_of_2_size {0x1000};

    // Cannot register MMIO region without write page
    BARETEST_ASSERT(not vm->register_mmio_region(dummy_addr, PAGE_SIZE));

    // Wrong API version fails
    BARETEST_ASSERT(
        not vm->register_write_page(mem->data.data(), default_max_timeout, default_max_requests, LFQ_API_VERSION + 1));

    // Too large outstanding requests fails
    BARETEST_ASSERT(not vm->register_write_page(mem->data.data(), default_max_timeout, PAGE_SIZE, LFQ_API_VERSION + 1));

    // Success
    BARETEST_ASSERT(
        vm->register_write_page(mem->data.data(), default_max_timeout, default_max_requests, LFQ_API_VERSION));

    // Registering twice fails
    BARETEST_ASSERT(
        not vm->register_write_page(mem->data.data(), default_max_timeout, default_max_requests, LFQ_API_VERSION));

    // Basic MMIO register/unregister works
    BARETEST_ASSERT(vm->register_mmio_region(aligned_address_4k, power_of_2_size));
    BARETEST_ASSERT(vm->unregister_mmio_region(aligned_address_4k, power_of_2_size));

    // Unaligned access but correct size fails
    BARETEST_ASSERT(not vm->register_mmio_region(aligned_address_4k + 8, power_of_2_size));

    // Size not a power of 2 fails
    BARETEST_ASSERT(not vm->register_mmio_region(aligned_address_4k, power_of_2_size * 3));

    // Unregister with a different size of the original region fails
    BARETEST_ASSERT(vm->register_mmio_region(aligned_address_4k, power_of_2_size));
    BARETEST_ASSERT(not vm->unregister_mmio_region(aligned_address_4k, power_of_2_size / 2));
}

TEST_CASE(posted_writes_shared_queue_receives_write_item)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    check_posted_writes(*vm);
}

TEST_CASE(posted_writes_destroy_and_reuse_vm_works)
{
    slvm_vm_id_t old_id {0};
    {
        auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};
        old_id = vm->vm_id;

        check_posted_writes(*vm);
    }
    {
        auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};
        // Make sure we get the same vm id in order to run in the same slvm-pd.
        // This way, we check that everything from our previous run was cleaned
        // up properly.
        BARETEST_ASSERT(vm->vm_id == old_id);

        check_posted_writes(*vm);
    }
}

TEST_CASE(posted_writes_two_vms_in_parallel_work)
{
    auto vm1 {std::make_unique<slvm_vm>(vm_mode::MODE64)};
    auto vm2 {std::make_unique<slvm_vm>(vm_mode::MODE64)};

    BARETEST_ASSERT(vm1->vm_id != vm2->vm_id);

    check_posted_writes(*vm1);
    check_posted_writes(*vm2);
}

TEST_CASE(ia32_tsc_aux_is_context_switched_between_slvm_and_ctrl_vm)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE16)};
    int exit_reason;

    static constexpr uint32_t AUX_VALUE_VM = 0x1337;

    vm->vcpu_state.tsc_aux = AUX_VALUE_VM;

    auto aux_before_vmlaunch = rdmsr(x86::IA32_TSC_AUX);

    vm->prepare_to_execute(ASM_BUFFER(guest_code_hlt));
    vm->vcpu_state.mtd.set(hedron::mtd_bits::TSC);
    exit_reason = vm->run_vcpu_ignore_exits();

    auto aux_after_vmlaunch = rdmsr(x86::IA32_TSC_AUX);

    BARETEST_ASSERT(expectation<uint32_t>(exit_reason) == VMX_PT_HLT);
    BARETEST_ASSERT(expectation<uint32_t>(vm->vcpu_state.tsc_aux) == AUX_VALUE_VM);
    BARETEST_ASSERT(expectation<uint32_t>(aux_before_vmlaunch) == aux_after_vmlaunch);
}

TEST_CASE(ia32_tsc_aux_is_context_switched_between_different_slvm_vms)
{
    auto vm1 {std::make_unique<slvm_vm>(vm_mode::MODE16)};
    auto vm2 {std::make_unique<slvm_vm>(vm_mode::MODE16)};

    static constexpr uint64_t AUX_VALUE_VM1 = 0x1337;
    static constexpr uint64_t AUX_VALUE_VM2 = 0x7331;

    vm1->prepare_to_execute(ASM_BUFFER(guest_code_hlt));
    vm2->prepare_to_execute(ASM_BUFFER(guest_code_hlt));

    vm1->vcpu_state.tsc_aux = AUX_VALUE_VM1;
    vm2->vcpu_state.tsc_aux = AUX_VALUE_VM2;
    vm1->vcpu_state.mtd.set(hedron::mtd_bits::TSC);
    vm2->vcpu_state.mtd.set(hedron::mtd_bits::TSC);

    vm1->run_vcpu_ignore_exits();
    vm2->run_vcpu_ignore_exits();

    vm1->vcpu_state.mtd.clear();
    vm2->vcpu_state.mtd.clear();
    vm1->run_vcpu_ignore_exits();
    vm2->run_vcpu_ignore_exits();

    BARETEST_ASSERT(expectation<uint32_t>(vm1->vcpu_state.tsc_aux) == AUX_VALUE_VM1);
    BARETEST_ASSERT(expectation<uint32_t>(vm2->vcpu_state.tsc_aux) == AUX_VALUE_VM2);
}

/**
 * We don't want to cache interrupt windows if we exit to the upstream VMM because:
 *  - we might have migrated between physical cores, so
 *    pending_interrupt_window information is stale
 *  - the upstream vmm might clear the interrupt window on the next
 *    exit because the event might not be pending anymore (e.g. if
 *    TPR has changed)
 *
 * While caching interrupt windows in this situation would be ok from a functional
 * perspective, it causes a lot of unnecessary exits that severely limit our
 * disk I/O performance.
 */
TEST_CASE(interrupt_window_should_not_be_cached_across_exits_to_upstream_vmm)
{
    auto vm {std::make_unique<slvm_vm>(vm_mode::MODE64)};
    consumer_queue q {*vm};

    // Register an MMIO posting page so we can emulate in SLVM-PD.
    // Using write posting in this test is necessary because we want
    // the SLVM-PD to emulate instructions for us without causing an exit.
    // Our assumption is that SLVM-PD won't cache any interrupt window state
    // as soon as we receive an exit here.
    const uintptr_t dummy_mmio_addr {0x10000};
    BARETEST_ASSERT(vm->register_mmio_region(dummy_mmio_addr, PAGE_SIZE));

    // We want to immediately exit after we have requested an interrupt window.
    // We simply let the guest execute hlt with rflags.IF=0 while requesting
    // an interrupt window.
    vm->prepare_to_execute(ASM_BUFFER(guest_code_hlt));
    vm->vcpu_state.rflags &= ~x86::FLAGS_IF;
    vm->vcpu_state.inj_info.request_interrupt_window();
    vm->vcpu_state.mtd.set(hedron::mtd_bits::INJ);

    // We expect to see a hlt exit because we configured the guest as not ready to receive interrupts.
    auto exit_reason_with_disabled_interrupts {vm->run_vcpu_ignore_exits()};
    BARETEST_ASSERT(expectation<uint64_t>(exit_reason_with_disabled_interrupts) == VMX_PT_HLT);

    // Be paranoid. We expect the injection info to be clear after the exits as interrupt
    // window requests are not transferred inbound. This ensures that no interrupt
    // window is requested on the next entry.
    BARETEST_ASSERT(expectation<bool>(vm->vcpu_state.inj_info.interrupt_window_requested()) == false);
    BARETEST_ASSERT(expectation<bool>(vm->vcpu_state.mtd.contains(hedron::mtd_bits::INJ)) == true);

    // New code that is executed on VMEntry. We first write to our posted-write location, then we
    // enable interrupts and execute hlt. We also need an additional instruction between enabling interrupts
    // and executing hlt because otherwise, we would be in STI_BLOCKING state exit with HLT even if an interrupt
    // windows would have been requested. If everything works as expected, we should not see any
    // interrupt windows as we didn't request one after the last exit.
    ASM_CODE(guest_code_mov_sti_hlt, "movl $0xc0fed00d, 0x10000; sti; nop; hlt;");
    vm->prepare_to_execute(ASM_BUFFER(guest_code_mov_sti_hlt));

    // We only want to see a hlt exit here, if we see an interrupt window then our interrupt window
    // caching in SLVM-PD is broken.
    BARETEST_ASSERT(vm->run_vcpu_ignore_exits() == VMX_PT_HLT);

    // Sanity check that we really emulated our mov instruction in SLVM-PD.
    const auto expected {posted_writes_descriptor {.addr = dummy_mmio_addr, .size = 4ul, .buf = 0xc0fed00d}};
    BARETEST_ASSERT(q.cons_q->front() == expected);
}
