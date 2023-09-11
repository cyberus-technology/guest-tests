/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/util/math.hpp>
#include <toyos/util/interval.hpp>
#include <toyos/util/order_range.hpp>

#include <optional>
#include <set>
#include <vector>

/**
 * \brief Generic block manager
 *
 * This block manager represents the allocation state by storing a set of free
 * blocks per order.
 */
class heap_block_manager
{
public:
   explicit heap_block_manager(math::order_t max_ord_)
      : max_ord(max_ord_)
   {
      ASSERT(max_ord <= maximal_order, "Bad maximal order");
      free_blocks.resize(max_ord + 1);
   }

   struct free_block_id;

   /**
     * Find a free block of order *at least* the provided one.
     *
     * \param order The order the free block should at least have.
     * \return An optional block id, empty optional if unsuccessful.
     */
   std::optional<free_block_id> get_free(math::order_t order) const
   {
      // Search for the minimal sufficient order for which a free block exists
      while (order <= max_ord and free_blocks[order].empty()) {
         ++order;
      }

      if (order > max_ord) {
         return {};
      }

      auto it{ free_blocks[order].begin() };
      return { { order, it } };
   }

   /**
     * Splits a free block in two halves and returns the first half.
     *
     * \param block The block to split.
     * \return The first half of the split block.
     */
   free_block_id split_free(const free_block_id& block)
   {
      math::order_t ord{ block.get_ord() };
      uintptr_t addr{ block.get_addr() };
      ASSERT(ord, "cannot split block of order 0");

      trace(TRACE_BUDDY, "Splitting order {} free block at addr {#x}", ord, addr);

      free_blocks[ord--].erase(block.it);

      auto res{ free_blocks[ord].emplace(addr + (static_cast<size_t>(1) << ord)) };
      ASSERT(res.second, "failed to insert right half of split block");

      res = free_blocks[ord].emplace(addr);
      ASSERT(res.second, "failed to insert left half of split block");

      return { ord, res.first };
   }

   /**
     * Merge a free block with its buddy, if possible.
     *
     * \param block The free block to merge.
     * \return Block id of the resulting block, empty optional otherwise.
     */
   std::optional<free_block_id> merge_free(const free_block_id& block)
   {
      // No merge if we're given a block of maximal order

      math::order_t ord{ block.get_ord() };
      if (ord >= max_ord) {
         return {};
      }

      // Check if there's another free block of the same order at buddy address

      uintptr_t addr{ block.get_addr() };
      uintptr_t buddy_addr{ get_buddy_addr(addr, ord) };

      auto& free_order_blocks{ free_blocks[ord] };

      auto buddy_it{ free_order_blocks.find(buddy_addr) };
      if (buddy_it == free_order_blocks.end()) {
         return {};
      }

      // Buddy is free of same order -> merge

      addr = std::min(addr, buddy_addr);

      free_order_blocks.erase(block.it);
      free_order_blocks.erase(buddy_it);

      auto res{ free_blocks[++ord].emplace(addr) };
      ASSERT(res.second, "Failed to insert merged free block");

      return { { ord, res.first } };
   }

   /**
     * Remove a free block.
     *
     * \param block The block to remove.
     */
   void mark_used(const free_block_id& block)
   {
      free_blocks[block.ord].erase(block.it);
   }

   /**
     * Add a free block (but don't do any merging).
     *
     * \param addr The address of the free block to add.
     * \param order The order of the free block.
     */
   free_block_id add_free(const uintptr_t addr, const math::order_t ord)
   {
      ASSERT(ord <= max_ord, "Order {} too large, maximum {}", ord, max_ord);
      ASSERT(math::is_aligned(addr, ord), "Address {#x} not aligned to order {}", addr, ord);
      ASSERT(free_blocks.size() > ord, "Vector with size {} too small for index {}", free_blocks.size(), ord);

      auto res{ free_blocks[ord].insert(addr) };
      ASSERT(res.second, "Failed to insert merged free block");

      return { ord, res.first };
   }

private:
   static constexpr uint64_t maximal_order{ 63 };

   /**
     * Calculate the address of the buddy for a given address and order.
     *
     * \param addr The address to search the buddy for.
     * \param order The order to search the buddy for.
     */
   uintptr_t get_buddy_addr(uintptr_t addr, math::order_t order)
   {
      ASSERT(order <= maximal_order, "Order {} too large, maximum {}", order, maximal_order);
      ASSERT(order <= max_ord, "Order {} too large, maximum {}", order, max_ord);
      return addr ^ (static_cast<uintptr_t>(1) << order);
   }

   using fixed_order_storage = std::set<uintptr_t>;
   using block_storage = std::vector<fixed_order_storage>;

   block_storage free_blocks;

   math::order_t max_ord{ 0 };

public:
   struct free_block_id
   {
      math::order_t ord;
      fixed_order_storage::const_iterator it;

      math::order_t get_ord() const
      {
         return ord;
      }
      uintptr_t get_addr() const
      {
         return *it;
      }
   };
};

/**
 * \brief A buddy allocator allows for the allocation and deallocation of naturally aligned 2-power address ranges.
 *
 * The interface used here differs from the usual alloc/free allocator interface in that free needs to be provided
 * the order of the address range to be deallocated - conceptually, this means that we are not keeping track of the
 * granularity of used regions. This simplifies the initialization and the size of the internal presentation.
 * The only drawback is that the user needs to know the order of the ranges to be freed.
 */
template<class block_manager = heap_block_manager>
class buddy_impl
{
public:
   /**
     * The buddy is created without memory, memory is added by free()-ing it.
     */
   explicit buddy_impl(uint64_t max_ord)
      : max_order(max_ord), blocks(max_order)
   {
      ASSERT(max_order <= maximal_order, "Bad maximal order");
   }

   /**
     * Requests a range the size of order.
     *
     * \param order The order to allocate.
     * \return Naturally aligned optional start address of the range if successful, empty optional otherwise.
     */
   std::optional<uintptr_t> alloc(math::order_t order)
   {
      trace(TRACE_BUDDY, "Allocation request of order {#x}", order);

      // Search for a sufficiently large free block

      auto opt_id{ blocks.get_free(order) };
      if (not opt_id) {
         trace(TRACE_BUDDY, "Out of memory");
         return {};
      }

      // Iteratively split free block until it has desired order

      free_block_id id{ *opt_id };
      trace(TRACE_BUDDY, "Choose order {} block at addr {#x}", id.get_ord(), id.get_addr());

      while (id.get_ord() > order) {
         id = blocks.split_free(id);
      }

      // Mark resulting block as used and return its address

      uintptr_t ret_addr{ id.get_addr() };

      ASSERT(id.get_ord() == order, "Block manager broken");
      blocks.mark_used(id);

      return { ret_addr };
   }

   /**
     * Frees an address range starting at addr with the size of the order.
     *
     * \param addr The start address of the address range to free.
     * \param order The order of the address range to free.
     */
   void free(uintptr_t addr, math::order_t order)
   {
      trace(TRACE_BUDDY, "Deallocation request of order {} at address {#x}", order, addr);

      // Mark block as free and merge free blocks as long as possible

      std::optional<free_block_id> opt_id{ blocks.add_free(addr, order) };
      ASSERT(opt_id, "no opt_id");

      while ((opt_id = blocks.merge_free(*opt_id))) {
         trace(TRACE_BUDDY, "Merged order {} buddies at address {#x}", opt_id->get_ord(), opt_id->get_addr());
      }
   }

   uint8_t max_order{ 0 };

private:
   static constexpr uint64_t maximal_order{ 63 };
   using free_block_id = typename block_manager::free_block_id;
   block_manager blocks;
};

using buddy = buddy_impl<>;

/**
 * Buddy helper function that adds the given memory interval as biggest possible chunks into the buddy.
 *
 * \param ival The address range to add to the buddy.
 * \param pool The buddy instance to add the memory to.
 */
template<class POOL>
inline void buddy_reclaim_range(const cbl::interval& ival, POOL& pool)
{
   for (auto range : cbl::order_range(ival, pool.max_order)) {
      pool.free(range.a, math::order_max(range.size()));
   }
}

template<class TYPE, class POOL>
inline void buddy_reclaim_range(const cbl::interval_impl<TYPE>& ival, POOL& pool)
{
   buddy_reclaim_range({ static_cast<uint64_t>(ival.a), static_cast<uint64_t>(ival.b) }, pool);
}
