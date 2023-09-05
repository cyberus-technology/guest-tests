/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <cbl/array_lock_free.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <optional>

/**
 * A lock free array data structure with reservable and swappable elements.
 *
 * The data structure is derived from the array_lock_free data structure and
 * allows to reserve slots in the array while keeping concurrent accesses
 * of multiple readers and writers allowed.
 * The concurrent reservation of array elements is allowed, too.
 * After clearing the array the write counter is reset to zero, too.
 *
 * Additionally, atomic swaps of existing values are possible.
 */
template <typename T, size_t SIZE, T INVALID_ELEMENT = T()>
class array_lock_free_reservable : private array_lock_free<T, SIZE, INVALID_ELEMENT>
{
public:
    using array_t = array_lock_free<T, SIZE, INVALID_ELEMENT>;
    using size_type = typename array_t::size_type;
    using value_type = typename array_t::value_type;

    array_lock_free_reservable() : array_t() { clear(); }

    /**
     * Obtain the value of the array at the given position.
     *
     * \param pos Position of requested value.
     *
     * \return Value at the given position.
     */
    value_type at(size_type pos) { return array_t::at(pos); }

    /**
     * Reserve an index for an element
     *
     * \return An optional index to the reserved slot.
               An empty optional indicates that the array is full no slot could be reserved.
     */
    std::optional<size_type> reserve_index()
    {
        const auto index {write_counter++};
        if (index >= SIZE) {
            write_counter--;
            return std::nullopt;
        }

        return index;
    }

    /**
     * Exchange an element in the array with a new one.
     *
     * \param position The position in the array that should be swapped.
     * \param value The value to be stored.
     *
     * \return The previous value stored at the position.
     */
    value_type swap(size_type position, const value_type& value)
    {
        assert((value != INVALID_ELEMENT) && "Inserting the invalid element is not allowed!");
        assert((position < write_counter.load()) && "Trying to insert index larger than the write counter!");

        return array_t::array[position].exchange(value);
    }

    /**
     * Obtain the first index of a stored value if it is contained.
     *
     * \param value Value to search the index for.
     *
     * \return Either the index if the element is found, or an empty optional
     */
    std::optional<size_type> find(const T& value) noexcept { return array_t::find(value); }

    /**
     * Erase the content of the array.
     * Please be aware that no other interaction with the array is allowed when clearing the array.
     */
    void clear()
    {
        array_t::clear();
        write_counter.store(0);
    }

    /**
     * Get the underlying array's size.
     *
     * \return size of the array.
     */
    size_type size() { return array_t::size(); }

private:
    std::atomic<size_type> write_counter;
};
