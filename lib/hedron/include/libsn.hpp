/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <arch.hpp>
#include <cbl/interval.hpp>
#include <hedron/cap_sel.hpp>
#include <hedron/hip.hpp>
#include <hedron/qpd.hpp>
#include <hedron/rights.hpp>
#include <hedron/syscalls.hpp>

#include <optional>

namespace hedron
{
class hip;
class utcb;
}; // namespace hedron

void libsn_init(const hedron::hip& hip);

/**
 * Reserves a consecutive range of
 * free (non-populated) pages of virtual memory.
 *
 * \param nr_pages number of pages to be reserved.
 * \return a pointer to the reserved memory region.
 */
page_num_t reserve_virtual_memory_pages(size_t nr_pages);

/** Reclaims a virtual memory range.
 *
 * \param ptr pages Interval of pages to be reclaimed.
 */
void reclaim_virtual_memory_pages(cbl::interval_impl<page_num_t> pages);

/**
 * Allocates a User Thread Control Block (UTCB).
 *
 * \return utcb pointer
 */
hedron::utcb* allocate_utcb();

/** Reclaims a User Thread Control Block (UTCB).
 *
 * \param utcb A UTCB pointer previously allocated with allocate_utcb.
 */
void reclaim_utcb(hedron::utcb* utcb);

/** Allocates a stack including a guard region
 * \return usable stack area as page numbers
 */
cbl::interval_impl<page_num_t> allocate_stack();

/** Reclaims a stack area that was allocated before.
 */
void reclaim_stack(cbl::interval_impl<page_num_t> ival);

/** Allocates memory that can be used for DMA transfers
 * \param ord size in order of pages
 * \return usable dma memory region as page numbers.
 */
cbl::interval allocate_dma_mem(size_t ord);

/** Reclaims and unmaps a DMA mapping from the virtual address space.
 */
void reclaim_dma_mem(cbl::interval_impl<page_num_t> ival);

/** Creates a semaphore at a given capability selector
 * \param selector capability selector the SM is created at
 * \param initial_counter the initial value of the semaphore counter
 */
void create_semaphore(hedron::cap_sel selector, uint64_t initial_counter);

/** Creates a EC at a given capability selector
 * \param selector capability selector the EC is created at
 * \param cpu the number of the CPU the EC will be bound to
 * \param utcb address of the utcb the EC will use
 * \param sp initial stackpointer of the EC
 * \param evt_base capability selector of the event handlers
 * \param global if true, EC will be a global EC
 * \param map_user_page_in_calling_pd If true, the UTCB will be mapped in the current PD and not in owner_pd.
 */
bool create_ec(std::optional<hedron::cap_sel> owner_pd, hedron::cap_sel selector, unsigned cpu, hedron::utcb* utcb,
               uintptr_t sp, hedron::cap_sel evt_base, bool global, bool map_user_page_in_calling_pd = false);

/** Creates a Portal at a given capability selector
 * \param selector capability selector the Portal is created at
 * \param ec the local EC the Portal will be bound to
 * \param ip address of the handler function of the Portal
 * \param mtd defines the architectural state that is transferred on Portal transition
 */
bool create_pt(hedron::cap_sel selector, hedron::cap_sel ec, uintptr_t ip, uint64_t mtd);

/** Creates a Scheduling Context at a given capability selector
 * \param selector capability selector the SC is created at
 * \param ec the global EC the SC will be bound to
 * \param qpd time quantum (quantum priority descriptor)
 */
bool create_sc(hedron::cap_sel selector, hedron::cap_sel ec, hedron::qpd qpd);

/** Creates a virtual memory region backed by anonoymous memory.
  * \param pn_range Range of page numbers.
  * \param rights   Rights to be applied on the regionlin_range.
  * \param return   True on success, false if memory region was
                    not empty before (FIXME: not implemented yet)
  */
bool map_anon_pn_range(cbl::interval_impl<page_num_t> pn_range, hedron::mem_rights rights,
                       hedron::space spaces = hedron::space::HOST);

/**
 * Allocates memory from the physical memory pool and identity maps it into the root PD.
 * \param size size in number of pages
 * \return optional interval that describes the allocated memory address range,
 *                  empty optional if allocation was unsuccessful
 */
std::optional<cbl::interval_impl<page_num_t>> allocate_anon_memory(size_t size);

/**
 * Frees the given anonymous memory interval and makes it available again.
 * \param interval the interval containing the address range to free
 */
void free_anon_memory(const cbl::interval_impl<page_num_t>& interval);

unsigned get_cpu_count();

/**
 * Retrieve the physical page number range of the roottask heap.
 */
cbl::interval_impl<page_num_t> get_heap_region();

/**
 * Obtain the list of multiboot modules
 */
const std::vector<hedron::hip::mem_descriptor> get_multiboot_modules();
