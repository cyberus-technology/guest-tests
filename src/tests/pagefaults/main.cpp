/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <toyos/baretest/baretest.hpp>
#include <toyos/testhelper/idt.hpp>
#include <toyos/testhelper/irq_handler.hpp>
#include <toyos/testhelper/irqinfo.hpp>
#include <toyos/util/literals.hpp>
#include <toyos/util/trace.hpp>
#include <toyos/x86/x86asm.hpp>

#include <toyos/mm.hpp>
#include <toyos/page_pool.hpp>
#include <toyos/pd.hpp>
#include <toyos/pdpt.hpp>
#include <toyos/pml4.hpp>
#include <toyos/testhelper/cr0_guard.hpp>
#include <toyos/testhelper/page_guard.hpp>
#include <toyos/util/cast_helpers.hpp>

using namespace x86;

// If we set this too low we might collide with our own heap or data sections.
const lin_addr_t TEST_ADDR{ lin_addr_t(uintptr_t(0xc00000)) };

static irqinfo irq_info;
jmp_buf jump_buffer;
page_pool pool;

static bool protection_violation(uint32_t err)
{
   return err & 0b001;
}
static bool caused_by_write(uint32_t err)
{
   return err & 0b010;
}
static bool caused_by_user_mode_access(uint32_t err)
{
   return err & 0b100;
}

using my_buddy_mem_t = std::array<char, 6 * PAGE_SIZE>;
alignas(PAGE_SIZE) my_buddy_mem_t my_buddy_mem;

static void irq_handler_fn(intr_regs* regs)
{
   irq_info.record(regs->vector, regs->error_code);
   longjmp(jump_buffer, 1);
}

void prologue()
{
   for(uint32_t i = 0; i < 6; ++i) {
      pool.free(phy_addr_t(ptr_to_num(&my_buddy_mem) + PAGE_SIZE * i));
   }

   PML4& pml4 = PML4::alloc(pool);
   PDPT& pdpt = PDPT::alloc(pool);
   pml4[0] = PML4E({ .address = uintptr_t(&pdpt), .present = true, .readwrite = true, .usermode = true });

   uint64_t lpage = 0;  // address for the large pages
   for(uint32_t i = 0; i < 4; ++i) {
      PD& pd = PD::alloc(pool);
      pdpt[i] = PDPTE::pdpte_to_pdir({ .address = uintptr_t(&pd), .present = true, .readwrite = true, .usermode = true });

      for(auto& e : pd) {
         e = PDE::pde_to_2mb_page({ .address = lpage, .present = true, .readwrite = true, .usermode = true });
         lpage += 2_MiB;  // add 2mb, cause its a large page
      }
   }

   memory_manager::set_pml4(pml4);
}

TEST_CASE(writing_to_unwriteable_page_with_cr0_wp_unset_should_not_cause_a_pagefault)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   cr0_guard cr0;
   set_cr0(get_cr0() & ~math::mask_from(cr0::WP));

   PDE& pde = memory_manager::pd_entry(TEST_ADDR);
   pde_guard pdeg(pde);

   pde.set_writeable(false, tlb_invalidation::yes);

   if(setjmp(jump_buffer) == 0) {
      *num_to_ptr<uint64_t>(TEST_ADDR) = 42;
   }

   BARETEST_ASSERT(irq_info.valid == false);
}

TEST_CASE(writing_to_unwriteable_page_with_cr0_wp_set_should_cause_a_pagefault)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   cr0_guard cr0;
   set_cr0(get_cr0() | math::mask_from(cr0::WP));

   PDE& pde = memory_manager::pd_entry(TEST_ADDR);
   pde_guard pdeg(pde);

   pde.set_writeable(false, tlb_invalidation::yes);

   if(setjmp(jump_buffer) == 0) {
      *num_to_ptr<uint64_t>(TEST_ADDR) = 42;
   }

   BARETEST_ASSERT(irq_info.valid == true);
   BARETEST_ASSERT(irq_info.vec == static_cast<uint32_t>(exception::PF));

   BARETEST_ASSERT(protection_violation(irq_info.err));
   BARETEST_ASSERT(caused_by_write(irq_info.err));
   BARETEST_ASSERT(not caused_by_user_mode_access(irq_info.err));
}

TEST_CASE(reading_from_unwriteable_page_should_not_cause_a_pagefault)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   PDE& pde = memory_manager::pd_entry(TEST_ADDR);
   pde_guard pdeg(pde);

   *num_to_ptr<uint64_t>(TEST_ADDR) = 42;
   pde.set_writeable(false, tlb_invalidation::yes);

   if(setjmp(jump_buffer) == 0) {
      BARETEST_ASSERT(*num_to_ptr<uint64_t>(TEST_ADDR) == 42);
   }

   BARETEST_ASSERT(irq_info.valid == false);
}

TEST_CASE(writing_to_not_present_page_should_cause_a_pagefault)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   PDE& pde = memory_manager::pd_entry(TEST_ADDR);
   pde_guard pdeg(pde);

   pde.set_present(false, tlb_invalidation::yes);

   if(setjmp(jump_buffer) == 0) {
      *num_to_ptr<uint64_t>(TEST_ADDR) = 42;
   }

   BARETEST_ASSERT(irq_info.valid == true);
   BARETEST_ASSERT(irq_info.vec == static_cast<uint32_t>(exception::PF));

   BARETEST_ASSERT(not protection_violation(irq_info.err));
   BARETEST_ASSERT(caused_by_write(irq_info.err));
   BARETEST_ASSERT(not caused_by_user_mode_access(irq_info.err));
}

TEST_CASE(reading_from_not_present_page_should_cause_a_pagefault)
{
   irq_handler::guard _(irq_handler_fn);
   irq_info.reset();

   PDE& pde = memory_manager::pd_entry(TEST_ADDR);
   pde_guard pdeg(pde);

   *num_to_ptr<uint64_t>(TEST_ADDR) = 42;
   pde.set_present(false, tlb_invalidation::yes);

   if(setjmp(jump_buffer) == 0) {
      BARETEST_ASSERT(*num_to_ptr<uint64_t>(TEST_ADDR) == 42);
   }

   BARETEST_ASSERT(irq_info.valid == true);
   BARETEST_ASSERT(irq_info.vec == static_cast<uint32_t>(exception::PF));

   BARETEST_ASSERT(not protection_violation(irq_info.err));
   BARETEST_ASSERT(not caused_by_write(irq_info.err));
   BARETEST_ASSERT(not caused_by_user_mode_access(irq_info.err));
}

BARETEST_RUN;
