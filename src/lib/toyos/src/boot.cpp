/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <compiler.hpp>
#include <config.hpp>

#include <codecvt>
#include <locale>
#include <string>
#include <vector>

#include <toyos/acpi.hpp>
#include <toyos/boot.hpp>
#include <toyos/boot_cmdline.hpp>
#include <toyos/cmdline.hpp>
#include <toyos/console/console_debugcon.hpp>
#include <toyos/console/console_serial.hpp>
#include <toyos/console/xhci_console.hpp>
#include <toyos/first-fit-heap/heap.hpp>
#include <toyos/memory/buddy.hpp>
#include <toyos/memory/simple_buddy.hpp>
#include <toyos/multiboot/multiboot.hpp>
#include <toyos/multiboot2/multiboot2.hpp>
#include <toyos/pci/bus.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>
#include <toyos/util/cpuid.hpp>
#include <toyos/util/interval.hpp>
#include <toyos/x86/arch.hpp>
#include <toyos/x86/segmentation.hpp>
#include <toyos/xen-pvh.hpp>
#include <toyos/xhci/debug_device.hpp>

extern int main();

extern uint64_t gdt_start asm("gdt");
extern uint64_t gdt_tss;

first_fit_heap<HEAP_ALIGNMENT>* current_heap{ nullptr };
simple_buddy* aligned_heap{ nullptr };

static constexpr size_t DMA_POOL_SIZE{ 0x100000 };
alignas(PAGE_SIZE) static char dma_pool_data[DMA_POOL_SIZE];
alignas(PAGE_SIZE) static x86::tss tss;  // alignment only used to avoid avoid crossing page boundaries
static buddy dma_pool{ 32 };

std::optional<boot_method> current_boot_method = std::nullopt;

std::string_view boot_method_name(boot_method method)
{
    switch (method) {
        case boot_method::MULTIBOOT1:
            return "Multiboot 1";
        case boot_method::MULTIBOOT2:
            return "Multiboot 2";
        case boot_method::XEN_PVH:
            return "Xen PVH";
    }
    __UNREACHED__
}

cbl::interval allocate_dma_mem(size_t ord)
{
    auto begin = dma_pool.alloc(ord);
    ASSERT(begin, "not enough DMA memory");

    return cbl::interval::from_order(*begin, ord);
}

EXTERN_C void init_heap()
{
    static constexpr unsigned HEAP_SIZE{ 1 /* MiB */ * 1024 * 1024 };
    alignas(CPU_CACHE_LINE_SIZE) static char heap_data[HEAP_SIZE];
    static fixed_memory heap_mem(size_t(heap_data), HEAP_SIZE);
    static first_fit_heap<HEAP_ALIGNMENT> heap(heap_mem);
    current_heap = &heap;
    static simple_buddy align_buddy{ 31 + math::order_max(HEAP_ALIGNMENT) };
    aligned_heap = &align_buddy;
}

/**
 * \brief Set up a 64 bit TSS.
 *
 * After activating IA-32e mode, we have to create a 64 bit TSS (Intel SDM, Vol. 3, 7.7).
 */
EXTERN_C void init_tss()
{
    static const uint16_t TSS_GDT_INDEX{ static_cast<uint16_t>(&gdt_tss - &gdt_start) };
    x86::segment_selector tss_selector{ static_cast<uint16_t>(TSS_GDT_INDEX << x86::segment_selector::INDEX_SHIFT) };
    x86::gdt_entry* gdte{ x86::get_gdt_entry(get_current_gdtr(), tss_selector) };

    gdte->set_system(true);  // TSS descriptor is a system descriptor
    gdte->set_g(false);      // interpret limit as bytes
    gdte->set_base(ptr_to_num(&tss));
    gdte->set_limit(sizeof(x86::tss));
    gdte->set_present(true);
    gdte->set_type(x86::gdt_entry::segment_type::TSS_64BIT_AVAIL);

    asm volatile("ltr %0" ::"r"(tss_selector.value()));
}

static std::u16string get_xhci_identifier(const std::string& arg)
{
    std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
    return converter.from_bytes(arg == "" ? "CBS0001" : arg);
}

static std::string boot_cmdline;
std::string get_boot_cmdline()
{
    return boot_cmdline;
}

/**
 * Returns the effective serial port by parsing the corresponding cmdline
 * option.
 */
uint16_t get_effective_serial_port(const std::string& serial_option, acpi_mcfg* mcfg)
{
    constexpr std::string_view HEX_PREFIX = "0x";

    if (serial_option.empty()) {
        return discover_serial_port(mcfg);
    }

    /**
     * Checks if the serial option has a prefix following a number. This doesn't
     * verify the number.
     */
    auto has_prefix = [serial_option](const std::string_view& needle) {
        auto len_prefix_plus_number = serial_option.length() > needle.length() + 1;
        auto starts_with = serial_option.compare(0, needle.length(), needle) == 0;
        return len_prefix_plus_number && starts_with;
    };

    // TODO once we switch to a more modern libcxx, std::stoull should do this
    //  automatically. The current version just aborts when a prefix is found.
    if (has_prefix(HEX_PREFIX)) {
        auto number_str = serial_option.substr(HEX_PREFIX.length());
        return std::stoull(number_str, nullptr, 16);
    }
    else {
        return std::stoull(serial_option, nullptr, 10);
    }
}

static void initialize_console(const std::string& cmdline, acpi_mcfg* mcfg)
{
    boot_cmdline = cmdline;

    cmdline::cmdline_parser p(cmdline, cmdline::optionparser::usage);

    auto serial_option = p.serial_option();
    auto xhci_option = p.xhci_option();

    if (serial_option.has_value()) {
        uint16_t port = get_effective_serial_port(serial_option.value(), mcfg);
        printf("Using serial port: %#x\n", port);
        serial_init(port);
    }
    else if (xhci_option) {
        // If we don't have an MCFG pointer here, there's not much we can do
        // to communicate. Let's just stop in a deterministic way.
        PANIC_UNLESS(mcfg, "No valid MCFG pointer given!");

        pci_bus pcibus(phy_addr_t(mcfg->base), mcfg->busses());

        auto is_xhci_fn = [](const pci_device& dev) { return dev.is_xhci(); };
        auto xhci_pci_dev = std::find_if(pcibus.begin(), pcibus.end(), is_xhci_fn);

        if (xhci_pci_dev != pcibus.end()) {
            const auto& pci_dev(*xhci_pci_dev);

            auto address = pci_dev.bar(0)->address();
            auto size = pci_dev.bar(0)->bar_size();

            auto mmio_region = cbl::interval::from_size(address, size);
            auto dma_region = pn2addr(allocate_dma_mem(math::order_envelope(XHCI_DMA_BUFFER_PAGES)));

            dummy_driver_adapter adapter;
            xhci_device xhci_dev(adapter, phy_addr_t(address));
            if (not xhci_dev.find_cap<xhci_debug_device::dbc_capability>()) {
                PANIC("No debug capability present!");
            }

            auto power_method = p.xhci_power_option() == "1" ? xhci_debug_device::power_cycle_method::POWERCYCLE : xhci_debug_device::power_cycle_method::NONE;
            static xhci_console_baremetal xhci_cons(get_xhci_identifier(*xhci_option), mmio_region, dma_region, power_method);
            xhci_console_init(xhci_cons);
        }
    }
    else {
        serial_init(discover_serial_port(mcfg));
    }

    // Some mainboards have a flaky serial, i.e. there is some noise on the line during bootup.
    // We work around this by simply sending a few newlines to have the actual data at the beginning of a line.
    printf("\n\n");
}

static void initialize_dma_pool()
{
    memset(dma_pool_data, 0, DMA_POOL_SIZE);

    auto dma_ival = cbl::interval::from_size(uintptr_t(&dma_pool_data), DMA_POOL_SIZE);
    buddy_reclaim_range(addr2pn(dma_ival), dma_pool);
}

// Doing a proper ACPI shutdown is not easy as one needs full ACPICA
// stack to parse the DSDT to find the proper ports. Hence, we just call
// some common shutdown ports.
// References:
// - https://wiki.osdev.org/Shutdown
// - https://github.com/qemu/qemu/blob/af9264da80073435fd78944bc5a46e695897d7e5/include/hw/xen/interface/hvm/params.h#L229
// - https://github.com/qemu/qemu/blob/af9264da80073435fd78944bc5a46e695897d7e5/tests/tcg/x86_64/system/boot.S#L133
// - Cloud Hypervisor: AcpiShutdownDevice
void __attribute__((noreturn)) shutdown()
{
    constexpr uint16_t CLOUD_HYPERVISOR_SHUTDOWN_PORT = 0x600;
    constexpr uint16_t CLOUD_HYPERVISOR_SHUTDOWN_VALUE = 0x34;
    constexpr uint16_t QEMU_SHUTDOWN_PORT = 0x604;
    constexpr uint16_t QEMU_SHUTDOWN_VALUE = 0x2000;
    constexpr uint16_t VIRTUALBOX_SHUTDOWN_PORT = 0x4004;
    constexpr uint16_t VIRTUALBOX_SHUTDOWN_VALUE = 0x3400;

    if (hv_bit_present()) {
        // Order is not important.
        // Unknown PIO writes are ignored by VMMs in general.
        outw(CLOUD_HYPERVISOR_SHUTDOWN_PORT, CLOUD_HYPERVISOR_SHUTDOWN_VALUE);
        outw(QEMU_SHUTDOWN_PORT, QEMU_SHUTDOWN_VALUE);
        outw(VIRTUALBOX_SHUTDOWN_PORT, VIRTUALBOX_SHUTDOWN_VALUE);
    }

    // If we can't shut down, at least HLT. This way, the guest tests won't
    // consume CPU time when one accidentally keeps a test running in a VMM
    // in background. On real hardware, the CPU stops spinning as well.
    disable_interrupts_and_halt();

    __UNREACHED__
}

EXTERN_C void init_interrupt_controllers()
{
    constexpr uint8_t PIC_BASE{ 32 };
    pic pic{ PIC_BASE };  // Initialize and mask PIC.

    ioapic ioapic{};

    // Mask all IRTs.
    for (uint8_t idx{ 0u }; idx < ioapic.max_irt(); idx++) {
        auto irt{ ioapic.get_irt(idx) };
        if (not irt.masked()) {
            irt.mask();
            ioapic.set_irt(irt);
        }
    }

    lapic_test_tools::software_apic_disable();
}

EXTERN_C void entry64(uint32_t magic, uintptr_t boot_info)
{
    // Init debugcon console as early as possible.
    if (hv_bit_present()) {
        console_debugcon::init();
    }

    initialize_dma_pool();

    std::string cmdline;
    acpi_mcfg* mcfg{ nullptr };

    if (magic == xen_pvh::MAGIC) {
        current_boot_method = boot_method::XEN_PVH;
        const auto* info = reinterpret_cast<xen_pvh::hvm_start_info*>(boot_info);
        cmdline = reinterpret_cast<const char*>(info->cmdline_paddr);

        const auto rsdp{ reinterpret_cast<const acpi_rsdp*>(info->rsdp_paddr) };
        mcfg = find_mcfg(rsdp);
    }
    else if (magic == multiboot::multiboot_module::MAGIC_LDR) {
        current_boot_method = boot_method::MULTIBOOT1;
        cmdline = reinterpret_cast<multiboot::multiboot_info*>(boot_info)->get_cmdline().value_or("");

        // On legacy systems (where we use Multiboot1), the ACPI tables can be
        // found with the legacy way (see find_mcfg()).
        mcfg = find_mcfg();
    }
    else if (magic == multiboot2::MB2_MAGIC) {
        current_boot_method = boot_method::MULTIBOOT2;
        auto reader{ multiboot2::mbi2_reader(reinterpret_cast<const uint8_t*>(boot_info)) };
        const auto cmdline_tag{ reader.find_tag(multiboot2::mbi2_cmdline::TYPE) };

        if (cmdline_tag) {
            const auto cmdline_full_tag{ cmdline_tag->get_full_tag<multiboot2::mbi2_cmdline>() };

            const auto* cmdline_begin{ cmdline_tag->addr + sizeof(cmdline_full_tag) };
            const auto* cmdline_end{ cmdline_tag->addr + cmdline_tag->generic.size };

            PANIC_UNLESS(cmdline_end >= cmdline_begin, "Malformed cmdline tag");
            std::copy(cmdline_begin, cmdline_end, std::back_inserter(cmdline));
        }

        // With UEFI firmware, the ACPI tables are passed to the kernel via the
        // Multiboot2 information structure. In order to use that, we extract
        // the embedded RSDP table and pass the pointer on to the discovery
        // function.
        const auto acpi_tag{ reader.find_tag(multiboot2::mbi2_rsdp2::TYPE) };

        if (acpi_tag) {
            const auto acpi_full_tag{ acpi_tag->get_full_tag<multiboot2::mbi2_rsdp2>() };
            const auto rsdp{ reinterpret_cast<const acpi_rsdp*>(acpi_tag->addr + sizeof(acpi_full_tag)) };
            mcfg = find_mcfg(rsdp);
        }
    }
    else {
        __builtin_trap();
    }

    initialize_console(cmdline, mcfg);

    main();

    shutdown();
}

void* operator new(size_t size)
{
    ASSERT(current_heap, "heap not initialized");
    auto tmp{ current_heap->alloc(size) };
    ASSERT(tmp, "out of memory");
    return tmp;
}

void* operator new(size_t size, std::align_val_t alignment)  // C++17, nodiscard in C++2a
{
    // use default heap if possible
    ASSERT(current_heap, "heap not initialized");
    if (alignment <= static_cast<std::align_val_t>(current_heap->alignment())) {
        return current_heap->alloc(size);
    }

    ASSERT(aligned_heap, "aligned heap not initialized");

    if (math::order_max(static_cast<size_t>(alignment)) > aligned_heap->max_order) {
        PANIC("Requested alignment bigger than available alignment {} > {}", static_cast<size_t>(alignment), aligned_heap->max_order);
        alignment = static_cast<std::align_val_t>(aligned_heap->max_order);
    }

    auto tmp = aligned_heap->alloc(math::order_max(std::max(size, static_cast<size_t>(alignment))));
    ASSERT(tmp, "Failed to allocate aligned memory.");
    return reinterpret_cast<void*>(*tmp);
}

void* operator new(size_t size, const std::nothrow_t&) noexcept  // nodiscard in C++2a
{
    return operator new(size);
}

void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept  // C++17, nodiscard in C++2a
{
    return operator new(size, alignment);
}

void* operator new[](size_t size)
{
    return operator new(size);
}

void* operator new[](size_t size, std::align_val_t alignment)  // C++17, nodiscard in C++2a
{
    return operator new(size, alignment);
}

void* operator new[](size_t size, const std::nothrow_t&) noexcept  // nodiscard in C++2a
{
    return operator new(size);
}

void* operator new[](size_t size, std::align_val_t alignment,
                     const std::nothrow_t&) noexcept  // C++17, nodiscard in C++2a
{
    return operator new(size, alignment);
}

void operator delete(void* p) noexcept
{
    ASSERT(current_heap, "heap not initialized");
    current_heap->free(p);
}

void operator delete(void* p, std::align_val_t alignment) noexcept  // C++17
{
    // use default heap if possible
    ASSERT(current_heap, "heap not initialized");
    if (alignment <= static_cast<std::align_val_t>(current_heap->alignment())) {
        return current_heap->free(p);
    }

    ASSERT(aligned_heap, "aligned heap not initialized");
    aligned_heap->free(reinterpret_cast<uintptr_t>(p));
}

void operator delete(void* p, size_t, std::align_val_t alignment) noexcept  // C++17
{
    operator delete(p, alignment);
}

void operator delete(void* p, const std::nothrow_t&) noexcept
{
    operator delete(p);
}

void operator delete(void* p, std::align_val_t alignment, const std::nothrow_t&) noexcept  // C++17
{
    operator delete(p, alignment);
}

void operator delete(void* p, size_t) noexcept
{
    operator delete(p);
}

void operator delete[](void* p) noexcept
{
    operator delete(p);
}

void operator delete[](void* p, size_t) noexcept
{
    operator delete[](p);
}

void operator delete[](void* p, std::align_val_t alignment) noexcept  // C++17
{
    operator delete(p, alignment);
}

void operator delete[](void* p, size_t, std::align_val_t alignment) noexcept  // C++17
{
    operator delete[](p, alignment);
}

void abort()
{
    PANIC("abort() called");
}
