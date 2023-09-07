/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "cassert"
#include "cstdint"
#include "cstdlib"
#include "cstring"
#include "initializer_list"
#include "limits"
#include "numeric"
#include "type_traits"

namespace math
{
using order_t = uint8_t;

/**
 * Generates a mask from a number of bitmasks.
 */
template <typename T, typename... ARGS, typename UT = std::underlying_type_t<T>>
constexpr std::underlying_type_t<T> mask_from(const T&& t, const ARGS&&... args)
{
    return (static_cast<UT>(t) | ... | static_cast<UT>(args));
}

/**
 * Checks whether a given value is a power of two, i.e. there exists a
 * value y where x = 2**y.
 */
static constexpr bool is_power_of_2(size_t x)
{
    return x != 0 && ((x - 1) & x) == 0;
}

/**
 * Calculates the order of the lowest bit that is set in a number.
 *
 * \param num number to calculate the order of the lowest bit set
 * \return the order of the lowest bit set in num
 */
static constexpr size_t order_min(size_t num)
{
    for (size_t i {0}; i < 8 * sizeof(num); ++i) {
        if (num & (1ull << i)) {
            return i;
        }
    }
    return 0;
}

/**
 * Calculates the order of the highest bit that is set in a number.
 *
 * \param num number to calculate the order of the highest bit set
 * \return the order of the highest bit set in num
 */
static constexpr size_t order_max(size_t num)
{
    for (size_t i {8 * sizeof(num) - 1}; i > 0; --i) {
        if (num & (1ull << i)) {
            return i;
        }
    }
    return 0;
}

/**
 * Calculates the smallest order that fulfills the following condition:
 * 1ul << order_envelope(num) >= num
 *
 * If num is a power of two, order_envelope == order_max == order_min.
 *
 * \param num number to find the order for
 * \return smallest order fulfilling the condition
 */
static constexpr size_t order_envelope(size_t num)
{
    return (1ul << order_max(num)) >= num ? order_max(num) : order_max(num) + 1;
}

/**
 * Generates a mask for a provided number of bits and shifts that mask by a given amount.
 * \param bits number of bits the mask should cover
 * \param offset number of bits to shift
 * \return effective mask
 */
static constexpr size_t mask(size_t bits, size_t offset = 0)
{
    const size_t m {bits == sizeof(size_t) * 8 ? ~0ull : (1ull << bits) - 1};

    return m << offset;
}

/**
 * Calculates an aligned value that is smaller than or equal to the provided one.
 * \param v value to be aligned
 * \param o order to align v to
 * \return aligned value
 */
template <typename T>
static constexpr T align_down(T v, order_t o)
{
    return v & ~mask(o);
}

/**
 * Calculates an aligned value that is greater than, or equal to the provided one.
 * \param v value to be aligned
 * \param o order to align v to
 * \return aligned value
 */
template <typename T>
static constexpr T align_up(T v, order_t o)
{
    return align_down(v + mask(o), o);
}

/**
 * Checks whether a value is aligned to a given order.
 */
template <typename T>
static constexpr bool is_aligned(T v, order_t o)
{
    return (v & mask(o)) == 0;
}

/**
 * Increments a 64bit unsigned value, skipping zero, and returns it.
 */
static constexpr uint64_t increment_uint64_without_zero(uint64_t val)
{
    return val + (~val == 0) + 1;
}

/**
 * Strict-Aliasing safe iterator type for consecutive ranges of trivially
 * copyable values
 *
 * Use this in situations where what you want is casting some pointer to T*
 * in order to iterate over it and dereference it. This would violate strict
 * aliasing rules - this iterator does not. Do not worry about performance:
 * The resulting assembly will not contain the memcpy calls that are used in
 * the dereference operator.
 */
template <typename T>
struct aliasing_safe_iterator
{
    static_assert(std::is_trivially_copyable<T>::value);
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = std::forward_iterator_tag;

    const char* p {nullptr};

    bool operator!=(const aliasing_safe_iterator& o) const { return p != o.p; }

    T operator*() const
    {
        T ret;
        memcpy(&ret, p, sizeof(T));
        return ret;
    }

    aliasing_safe_iterator& operator++()
    {
        p += sizeof(T);
        return *this;
    }
};

/**
 * Calculates a checksum for a piece of memory. The checksum is calculated by
 * accumulating all values inside memory using a provided type and subtracting the
 * result from zero.
 * \param base base address of the memory the checksum should be calculated for
 * \param sz size in bytes
 * \return calculated checksum
 */
template <typename T>
T checksum(const void* const base, size_t size)
{
    static_assert(std::is_unsigned<T>::value, "this function works for unsigned types only");
    assert(size % sizeof(T) == 0);
    const char* p {static_cast<const char*>(base)};
    aliasing_safe_iterator<T> begin {p};
    aliasing_safe_iterator<T> end {p + size};
    return T {0} - std::accumulate(begin, end, T {0});
}

/*
 * Determines whether the type of base overflows when adding limit to base
 */
template <typename T, typename U>
static constexpr bool will_overflow(T base, U limit)
{
    static_assert(std::is_unsigned<T>::value, "this function works for unsigned types only");
    static_assert(std::is_unsigned<U>::value, "this function works for unsigned types only");
    return std::numeric_limits<T>::max() - base < limit;
}

} // namespace math
