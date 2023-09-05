/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <array>
#include <atomic>
#include <cbl/cast_helpers.hpp>
#include <cbl/in_place_atomic.hpp>
#include <cbl/page_pool.hpp>
#include <math.hpp>

enum class tlb_invalidation : bool
{
    yes = true,
    no = false,
};

/* The base class for the paging entries. Getting and setting values are atomic.
 */
class paging_entry_base
{
protected:
    enum
    {
        PR_SHIFT = 0,    // Present
        RW_SHIFT = 1,    // Read/write
        US_SHIFT = 2,    // User/supervisor
        PWT_SHIFT = 3,   // Page-level write-through
        PCD_SHIFT = 4,   // Page-level cache-disable
        A_SHIFT = 5,     // Accessed
        ADDR_SHIFT = 12, // Start of the bits used for the address of the pdpt, pd, whatever
        XD_SHIFT = 63,   // execute-disable

        ADDR_BITS = 40,

        PR_MASK = math::mask(1, PR_SHIFT),
        RW_MASK = math::mask(1, RW_SHIFT),
        US_MASK = math::mask(1, US_SHIFT),
        PWT_MASK = math::mask(1, PWT_SHIFT),
        PCD_MASK = math::mask(1, PCD_SHIFT),
        A_MASK = math::mask(1, A_SHIFT),
        ADDR_MASK = math::mask(ADDR_BITS, ADDR_SHIFT),
        XD_MASK = math::mask(1, XD_SHIFT),
    };

    /* Every paging entry holds an 64bit value. you can create an entry by giving it the value
     * it should hold, you can give it a config (please have a look into the pml4.hpp, pdpt.hpp,
     * pd.hpp or pt.hpp) or using the copy constructor.
     */
    paging_entry_base() = default;
    explicit paging_entry_base(uint64_t raw) : raw_(raw) {}
    explicit paging_entry_base(const paging_entry_base& other) { exchange_raw_atomic(other, *this); }

public:
    bool is_present() const { return raw() & PR_MASK; }
    bool is_writeable() const { return is_present() and (raw() & RW_MASK); }
    bool is_usermode() const { return is_present() and (raw() & US_MASK); }
    bool is_pwt() const { return is_present() and (raw() & PWT_MASK); }
    bool is_pcd() const { return is_present() and (raw() & PCD_MASK); }
    bool is_accessed() const { return is_present() and (raw() & A_MASK); }
    bool is_exec_disable() const { return is_present() and (raw() & XD_MASK); }

    explicit operator uint64_t() const { return raw(); }
    bool operator==(const paging_entry_base& other) { return raw() == other.raw(); }

    paging_entry_base& operator=(const paging_entry_base& other)
    {
        exchange_raw_atomic(other, *this);
        return *this;
    }

protected:
    void set_bits(uint64_t clr_mask, uint64_t set_mask)
    {
        uint64_t expected = 0;
        uint64_t desired = 0;
        do {
            expected = raw();
            desired = (expected & ~clr_mask) | set_mask;
        } while (cbl::in_place_atomic<uint64_t>(raw_).compare_exchange_strong(
                     expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst)
                 == false);
    }

    static void exchange_raw_atomic(const paging_entry_base& src, paging_entry_base& dest)
    {
        uint64_t expected = 0;
        uint64_t desired = 0;
        do {
            expected = dest.raw();
            desired = src.raw();
        } while (cbl::in_place_atomic<uint64_t>(dest.raw_).compare_exchange_strong(
                     expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst)
                 == false);
    }

    uint64_t raw() const { return cbl::in_place_atomic<const uint64_t>(raw_).load(); }
    uint64_t raw_;
};

/* The base class for all the containers that will hold the paging entries. you can call the default
 * constructor, or give it a reference to a page pool, and the structure will allocate an address by itself.
 */
template <class ENTRY>
struct paging_structure_container
    : uncopyable
    , unmovable
{
    paging_structure_container() = default;

    static paging_structure_container& alloc(page_pool& pool)
    {
        return *(new (num_to_ptr<void>(pool.alloc())) paging_structure_container);
    }

    using entry_container_t = std::array<ENTRY, 512>;
    using iterator_t = typename entry_container_t::iterator;

    iterator_t begin() { return internal_table.begin(); }
    iterator_t end() { return internal_table.end(); }

    iterator_t cbegin() const { return internal_table.cbegin(); }
    iterator_t cend() const { return internal_table.cend(); }

    ENTRY& at(size_t idx) { return internal_table.at(idx); }
    ENTRY& operator[](size_t idx) { return internal_table[idx]; }

private:
    entry_container_t internal_table;
};
