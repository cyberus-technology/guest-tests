/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/interval.hpp>
#include <cbl/traits.hpp>
#include <math.hpp>
#include <trace.hpp>
#include <x86asm.hpp>
#include <x86defs.hpp>

constexpr uint64_t addr2pn(uint64_t addr)
{
    return addr >> PAGE_BITS;
}
constexpr uint64_t pn2addr(uint64_t pn)
{
    return pn << PAGE_BITS;
}

constexpr bool is_page_aligned(uint64_t addr)
{
    return math::is_aligned(addr, PAGE_BITS);
}

constexpr cbl::interval pn2addr(cbl::interval ival)
{
    return {pn2addr(ival.a), pn2addr(ival.b)};
}

constexpr size_t pages_to_size(size_t pages)
{
    return pn2addr(cbl::interval::from_size(0, pages)).size();
}

namespace x86
{

/**
 * Retrieve and cache the largest extended function code that is supported by the cpuid instruction.
 *
 * \return Largest extended function code value.
 */
uint32_t get_largest_extended_function_code();

/**
 * Returns the number of bits available for a linear address.
 */
uint64_t get_max_lin_addr_bits();

/**
 * Returns the number of bits available for a physical address.
 */
uint64_t get_max_phy_addr_bits();

} // namespace x86

/**
 * Returns a mask for the upper half of the canonical address space.
 */
inline uint64_t get_canonical_mask()
{
    auto number_bits {x86::get_max_lin_addr_bits()};
    return math::mask(cbl::bit_width<void*>() - (number_bits - 1), number_bits - 1);
}

class generic_addr_t
{
public:
    generic_addr_t() = delete;

    explicit constexpr generic_addr_t(uintptr_t addr_) : addr(addr_) {}

    bool operator!=(const std::nullptr_t&) const { return addr != 0; }
    bool operator==(const std::nullptr_t&) const { return not operator!=(nullptr); }
    explicit constexpr operator uintptr_t() const { return addr; }

protected:
    uintptr_t addr;
};

class page_num_t : public generic_addr_t
{
public:
    explicit constexpr page_num_t(uint64_t page_number_ = 0) : generic_addr_t(page_number_) {}

    static constexpr page_num_t from_address(uint64_t addr_) { return page_num_t {addr2pn(addr_)}; }

    constexpr page_num_t operator+(const page_num_t& o) const { return page_num_t {this->addr + o.addr}; }
    constexpr page_num_t operator-(const page_num_t& o) const { return page_num_t {this->addr - o.addr}; }
    constexpr page_num_t operator%(const page_num_t& o) const { return page_num_t {this->addr % o.addr}; }

    constexpr page_num_t& operator+=(const page_num_t& o)
    {
        this->addr += o.addr;
        return *this;
    }

    constexpr page_num_t& operator++()
    {
        ++addr;
        return *this;
    }

    constexpr bool operator<(const page_num_t& o) const { return this->addr < o.addr; }
    constexpr bool operator>(const page_num_t& o) const { return this->addr > o.addr; }

    constexpr bool operator<=(const page_num_t& o) const { return this->addr <= o.addr; }
    constexpr bool operator>=(const page_num_t& o) const { return this->addr >= o.addr; }

    constexpr bool operator==(const page_num_t& o) const { return this->addr == o.addr; }
    constexpr bool operator!=(const page_num_t& o) const { return not operator==(o); };

    constexpr uint64_t to_address() const { return pn2addr(addr); }
    constexpr uint64_t raw() const { return addr; }

    template <class T>
    constexpr T* to_ptr() const
    {
        return reinterpret_cast<T*>(to_address());
    }
};

class lin_addr_t : public generic_addr_t
{
public:
    explicit constexpr lin_addr_t(uintptr_t addr_) : generic_addr_t(addr_) {}

    page_num_t pn() { return page_num_t::from_address(this->addr); }

    lin_addr_t operator+(size_t bytes) const { return lin_addr_t(addr + bytes); }

    bool operator<(const lin_addr_t& rhs) const { return addr < rhs.addr; }
    bool operator==(const lin_addr_t& rhs) const { return addr == rhs.addr; }
};

class phy_addr_t : public generic_addr_t
{
public:
    explicit constexpr phy_addr_t(uintptr_t addr_) : generic_addr_t(addr_) {}

    phy_addr_t operator+=(size_t bytes)
    {
        addr += bytes;
        return *this;
    }

    bool operator<(const phy_addr_t& o) const { return addr < o.addr; }
    bool operator>(const phy_addr_t& o) const { return addr > o.addr; }
    bool operator>=(const phy_addr_t& o) const { return addr >= o.addr; }
    bool operator<=(const phy_addr_t& o) const { return addr <= o.addr; }
    bool operator!=(const phy_addr_t& o) const { return o.addr != addr; }
    bool operator!=(const std::nullptr_t&) const { return addr != 0; }
    bool operator==(const std::nullptr_t&) const { return not operator!=(nullptr); }
    bool operator==(const phy_addr_t& o) const { return not operator!=(o); }

    page_num_t pfn() { return page_num_t::from_address(this->addr); };

    phy_addr_t operator+(size_t bytes) const { return phy_addr_t(addr + bytes); }
    phy_addr_t operator-(size_t bytes) const { return phy_addr_t(addr - bytes); }
    phy_addr_t operator-(phy_addr_t other) const { return phy_addr_t(addr - other.addr); }
};

struct log_addr_t
{
    explicit log_addr_t(x86::segment_register seg_, uintptr_t off_) : seg(seg_), off(off_) {}

    log_addr_t() = delete;

    log_addr_t operator+(size_t bytes) const { return log_addr_t(seg, off + bytes); }
    log_addr_t operator-(size_t bytes) const { return log_addr_t(seg, off - bytes); }

    bool operator==(const log_addr_t& o) const { return seg == o.seg and off == o.off; }
    bool operator!=(const log_addr_t& o) const { return not operator==(o); };

    x86::segment_register seg;
    uintptr_t off;
};

constexpr cbl::interval_impl<page_num_t> addr2pn(const cbl::interval& ival)
{
    return {page_num_t::from_address(ival.a), page_num_t::from_address(math::align_up(ival.b, PAGE_BITS))};
}

constexpr cbl::interval pn2addr(const cbl::interval_impl<page_num_t>& pn_ival)
{
    return {pn_ival.a.to_address(), pn_ival.b.to_address()};
}

constexpr uint64_t pn2addr(page_num_t pn)
{
    return pn.to_address();
}

constexpr size_t size_to_pages(size_t size)
{
    return addr2pn(cbl::interval::from_size(0, size)).size().raw();
}
