/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <map>

#include <toyos/util/trace.hpp>

#include <toyos/testhelper/int_guard.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/testhelper/lapic_test_tools.hpp>
#include <toyos/testhelper/pic.hpp>
#include <toyos/util/cast_helpers.hpp>
#include <toyos/x86/cpuid.hpp>
#include <toyos/x86/x86asm.hpp>
#include <toyos/x86/x86defs.hpp>

#include <toyos/baretest/baretest.hpp>

#include "x2apic_test_tools.hpp"

using namespace lapic_test_tools;
using namespace x2apic_test_tools;
using namespace x86;

static constexpr uint8_t PIC_BASE{ 0x30 };
static pic global_pic{ PIC_BASE };

mmio_buffer_t mmio_buffer{ x2apic_msrs };

// for test raising gpes
struct irqinfo irq_info;
jmp_buf jump_buffer;

static void irq_handler_fn(intr_regs* regs)
{
   irq_info.record(regs->vector, regs->error_code);
   // the gpe does not need an EOI
   if(regs->vector != to_underlying(exception::GP)) {
      if(not x2apic_mode_enabled()) {
         send_eoi();
      }
      else {
         write_x2_msr(msr::X2APIC_EOI, 0);
      }
   }
   longjmp(jump_buffer, 1);
}

void prologue()
{
   irq_handler::guard _(drain_irq);
   enable_interrupts_for_single_instruction();

   // BIOS leaves APIC in an unknown state
   // set it to a well defined state with a reset
   global_apic_disable();
   global_apic_enable();
}

TEST_CASE(lapic_enabled)
{
   BARETEST_ASSERT(global_apic_enabled());
}

TEST_CASE(lapic_at_default_addr)
{
   static constexpr uint32_t MSR_APIC_BASE_MASK_INVERTED{ 0xFFF };
   // if this does not match, we read the buffered data from
   // another address and everything is meaningless...
   auto msr_val{ rdmsr(msr::IA32_APIC_BASE) };
   auto base_addr{ msr_val & (~MSR_APIC_BASE_MASK_INVERTED) };
   BARETEST_ASSERT(base_addr == LAPIC_START_ADDR);
}

TEST_CASE(lapic_lvt_entries_when_apic_global_disabled)
{
   int_guard _;
   global_apic_disable();

   uint32_t DISABLED_LVT{ ~0u };
   for(const auto reg : lvt_regs) {
      BARETEST_ASSERT(read_from_register(reg) == DISABLED_LVT);
   }

   global_apic_enable();
   software_apic_enable();
}

TEST_CASE(lapic_lvt_entries_when_apic_software_disabled)
{
   int_guard _;
   software_apic_disable();

   uint32_t MASKED_LVT{ LVT_MASK_MASK << LVT_MASK_SHIFT };
   for(const auto reg : lvt_regs) {
      BARETEST_ASSERT(read_from_register(reg) == MASKED_LVT);
   }

   software_apic_enable();
}

TEST_CASE(lapic_lvt_entry_change_when_apic_global_disabled)
{
   int_guard _;
   global_apic_disable();

   // lets write a custom vector to LINT0
   static constexpr uint8_t VECTOR{ 33 };
   auto old_lint0 = read_from_register(LAPIC_LVT_LINT0);
   auto new_lint0 = (old_lint0 & ~LVT_VECTOR_MASK) | VECTOR;

   write_to_register(LAPIC_LVT_LINT0, new_lint0);
   // the write should be ignored
   BARETEST_ASSERT(read_from_register(LAPIC_LVT_LINT0) == old_lint0);

   global_apic_enable();
}

TEST_CASE(lapic_lvt_entry_change_when_apic_software_disabled)
{
   int_guard _;
   software_apic_disable();

   // lets write a custom vector to LINT0
   static constexpr uint8_t VECTOR{ 33 };
   auto old_lint0 = read_from_register(LAPIC_LVT_LINT0);
   auto new_lint0 = (old_lint0 & ~LVT_VECTOR_MASK) | VECTOR;

   write_to_register(LAPIC_LVT_LINT0, new_lint0);
   // the write should not be ignored
   BARETEST_ASSERT(read_from_register(LAPIC_LVT_LINT0) == new_lint0);

   // we change the masked bit - this should be ignored
   write_to_register(LAPIC_LVT_LINT0, new_lint0 | (LVT_MASK_MASK << LVT_MASK_SHIFT));
   BARETEST_ASSERT(read_from_register(LAPIC_LVT_LINT0) == new_lint0);

   // restore old state
   write_to_register(LAPIC_LVT_LINT0, old_lint0);

   software_apic_enable();
}

/*
 * When the APIC is disabled, CPUID has to report that the APIC is
 * absent.
 *
 * This test fails on supernova as long as issue 295
 * (https://gitlab.vpn.cyberus-technology.de/supernova-core/supernova-core/issues/295)
 * is not resolved.
 * Once the issue is fixed, the test can be used.
 */
/* TEST_CASE(apic_disabled_matches_cpuid)
{
    static constexpr uint32_t CPUID_BASIC_01 {1};
    static constexpr uint32_t EDX_APIC_SHIFT {9};
    static constexpr uint32_t EDX_APIC_MASK  {1u << EDX_APIC_SHIFT};

    // at first, apic is present
    BARETEST_ASSERT(cpuid(CPUID_BASIC_01).edx & EDX_APIC_MASK);

    // we do not want an interrupt while xapic is disabled
    int_guard _;

    disable_apic();

    // apic should not be present anymore
    BARETEST_ASSERT(not (cpuid(CPUID_BASIC_01).edx & EDX_APIC_MASK));

    enable_apic();

    // apic should be present again
    BARETEST_ASSERT(cpuid(CPUID_BASIC_01).edx & EDX_APIC_MASK);
}
*/

static bool read_x2msr_raises_gpe(const x2apic_msr& msr)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   if(setjmp(jump_buffer) == 0) {
      msr.read();
   }
   return irq_info.valid and irq_info.vec == to_underlying(exception::GP);
}

static bool write_x2msr_raises_gpe(const x2apic_msr& msr, uint64_t value)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   if(setjmp(jump_buffer) == 0) {
      write_x2_msr(msr.addr, value);
   }
   return irq_info.valid and irq_info.vec == to_underlying(exception::GP);
}

TEST_CASE_CONDITIONAL(x2apic_msr_access_out_of_x2mode_raises_gpe, x2apic_mode_supported())
{
   for(const auto& msr : x2apic_msrs) {
      BARETEST_ASSERT(read_x2msr_raises_gpe(msr));
   }

   for(const auto& msr : x2apic_msrs) {
      BARETEST_ASSERT(write_x2msr_raises_gpe(msr, 0));
   }
}

TEST_CASE_CONDITIONAL(x2apic_mode_enable_disable, x2apic_mode_supported())
{
   BARETEST_ASSERT(not x2apic_mode_enabled());
   {
      x2apic_mode_guard guard;
      BARETEST_ASSERT(x2apic_mode_enabled());
   }
   BARETEST_ASSERT(not x2apic_mode_enabled());
}

TEST_CASE_CONDITIONAL(x2apic_initial_preserved_values, x2apic_mode_supported())
{
   x2apic_mode_guard guard{ &mmio_buffer };

   // check that register values are preserved
   for(const auto& msr : x2apic_msrs) {
      if(msr.is_readable() and msr.is_preserved_on_init()) {
         auto mmio_addr{ msr.mmio_addr() };
         // if it is preserved on init, there has to be an mmio register value
         BARETEST_ASSERT(mmio_addr);

         uint32_t buffered_val{ mmio_buffer.get(msr) };
         uint64_t x2_val{ read_x2_msr(msr.addr) };
         BARETEST_ASSERT(buffered_val == x2_val);
      }
   }
}

TEST_CASE_CONDITIONAL(x2apic_initial_ldr_values, x2apic_mode_supported())
{
   // xapic apic id
   uint32_t initial_apic_id{ read_from_register(LAPIC_ID) };

   static constexpr uint32_t XAPIC_APIC_ID_MASK{ 0xFF };
   static constexpr uint32_t XAPIC_APIC_ID_SHIFT{ 24 };
   initial_apic_id = initial_apic_id & (XAPIC_APIC_ID_MASK << XAPIC_APIC_ID_SHIFT);

   x2apic_mode_guard guard;

   static constexpr uint32_t LOW_MASK{ 0xF };
   static constexpr uint32_t HIGH_MASK{ 0xFFF };
   static constexpr uint32_t HIGH_SHIFT{ 16 };

   uint32_t ldr_new = (initial_apic_id & (HIGH_MASK << HIGH_SHIFT)) | (1 << (initial_apic_id & LOW_MASK));

   // read apic id in x2mode
   BARETEST_ASSERT(ldr_new == read_x2_msr(msr::X2APIC_LDR));
}

TEST_CASE_CONDITIONAL(x2apic_write_readonly_raises_gpe, x2apic_mode_supported())
{
   x2apic_mode_guard guard;
   static constexpr uint64_t VALUE{ 0 };
   for(const auto& msr : x2apic_msrs) {
      if(not msr.is_writeable()) {
         BARETEST_ASSERT(write_x2msr_raises_gpe(msr, VALUE));
      }
   }
}

TEST_CASE_CONDITIONAL(x2apic_read_writeonly_raises_gpe, x2apic_mode_supported())
{
   x2apic_mode_guard guard;
   for(const auto& msr : x2apic_msrs) {
      if(not msr.is_readable()) {
         BARETEST_ASSERT(read_x2msr_raises_gpe(msr));
      }
   }
}

/*
 * Writes a 64 bit value to all writable 32 bit registers.
 * 64 bit registers do not cause a GPE when written, but
 * the writes changes the APIC's state. So we can not
 * check whether they do NOT raise a GPE here.
 */
TEST_CASE_CONDITIONAL(x2apic_write_high_32bit_raises_gpe, x2apic_mode_supported())
{
   x2apic_mode_guard guard;
   // 64 bit value with one of the higher 32 bit set to 1
   static constexpr uint64_t HIGH_VALUE{ 1ul << 32u };
   for(const auto& msr : x2apic_msrs) {
      if(msr.is_32bit() and msr.is_writeable()) {
         BARETEST_ASSERT(write_x2msr_raises_gpe(msr, HIGH_VALUE));
      }
   }
}

TEST_CASE_CONDITIONAL(x2apic_self_ipi, x2apic_mode_supported())
{
   // while messing with tpr, we do not want interrupts
   int_guard _;
   // switch to x2apic mode
   x2apic_mode_guard x2guard;

   // adjust priority so that all interrupts can be delivered
   auto tpr_old{ read_x2_msr(msr::X2APIC_TPR) };
   write_x2_msr(msr::X2APIC_TPR, 0);

   // register handler for self_ipi
   irq_handler::guard irq_guard(irq_handler_fn);
   irq_info.reset();

   // vectors 32 - 255 are free for use
   static constexpr uint64_t vector{ 32 };

   // a spurious interrupt should not raise the exception we are expecting
   uint8_t spurious_vector = read_x2_msr(msr::X2APIC_SVR) & SVR_VECTOR_MASK;
   BARETEST_ASSERT(vector != spurious_vector);

   if(setjmp(jump_buffer) == 0) {
      write_x2_msr(msr::X2APIC_X2_SELF_IPI, vector);
      asm("sti; hlt; cli");
   }

   BARETEST_ASSERT(irq_info.valid);
   BARETEST_ASSERT(irq_info.vec == vector);

   // restore priority
   write_x2_msr(msr::X2APIC_TPR, tpr_old);
}

TEST_CASE(invalid_apic_mode)
{
   int_guard _;
   irq_handler::guard irq_guard(irq_handler_fn);
   irq_info.reset();

   if(setjmp(jump_buffer) == 0) {
      // disable apic and enable x2apic mode
      global_apic_disable();
      x2apic_mode_enable();
   }
   BARETEST_ASSERT(irq_info.valid);
   BARETEST_ASSERT(irq_info.vec == to_underlying(exception::GP));

   // turns on apic
   x2apic_mode_disable();
}

TEST_CASE(apic_transition_disabled_to_x2mode)
{
   int_guard _;
   irq_handler::guard irq_guard(irq_handler_fn);
   irq_info.reset();

   if(setjmp(jump_buffer) == 0) {
      // disable apic
      global_apic_disable();

      // switch to en = 1 and ext = 1
      auto msr_val{ rdmsr(msr::IA32_APIC_BASE) };
      msr_val |= APIC_GLOBAL_ENABLED_MASK << APIC_GLOBAL_ENABLED_SHIFT;
      msr_val |= X2APIC_ENABLED_MASK << X2APIC_ENABLED_SHIFT;
      write_x2_msr(msr::IA32_APIC_BASE, msr_val);
   }
   BARETEST_ASSERT(irq_info.valid)
   BARETEST_ASSERT(irq_info.vec == to_underlying(exception::GP));

   // turns on apic
   x2apic_mode_disable();
}

BARETEST_RUN;
