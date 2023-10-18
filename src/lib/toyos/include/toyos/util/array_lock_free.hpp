/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <optional>

/**
 * A lock free vector data structure with a static size.
 * Concurrent accesses of multiple readers and multiple concurrent insertions are allowed.
 */
template<typename T, size_t SIZE, T INVALID_ELEMENT = T()>
class array_lock_free
{
 public:
    using size_type = size_t;
    using value_type = T;

    array_lock_free()
    {
        clear();
    }

    /**
     * Obtain the value of the array at the given position.
     *
     * \param pos Position of requested value.
     *
     * \return Value at the given position.
     */
    value_type at(size_type pos)
    {
        return array.at(pos).load();
    }

    /**
     * Obtain the first index of a stored value if it is contained.
     *
     * \param value Value to search the index for.
     *
     * \return Either the index if the element is found, or an empty optional
     */
    std::optional<size_type> find(const T& value) noexcept
    {
        return find_if([value](const auto& elem) { return elem == value; });
    }

    /**
     * Obtain the first index of a stored element if it fulfills the given
     * condition.
     *
     * \param cond Condition that the found element must fulfill.
     *
     * \return Either the index if an element is found, or an empty optional
     */
    template<typename FUNC>
    std::optional<size_type> find_if(FUNC cond) noexcept
    {
        auto it{ std::find_if(array.begin(), array.end(), [cond](const std::atomic<T>& array_value) { return cond(array_value.load()); }) };
        if (it != array.end()) {
            return std::distance(array.begin(), it);
        }
        return {};
    }

    /**
     * Insert an element.
     *
     * \param value Value to be stored.
     *
     * \return index if the value can be inserted empty optional if not.
     */
    std::optional<size_type> insert(const T& value)
    {
        assert((value != INVALID_ELEMENT) && "Inserting the invalid element is not allowed!");

        auto exchange_fn = [&value](std::atomic<T>& array_value) -> bool {
            auto null_element{ INVALID_ELEMENT };
            return array_value.compare_exchange_strong(null_element, value);
        };

        auto it{ std::find_if(array.begin(), array.end(), exchange_fn) };

        if (it != array.end()) {
            return std::distance(array.begin(), it);
        }

        return {};
    }

    /**
     * Erase the content of the array.
     * Please be aware that no other interaction to the array is allowed when clearing the array!
     */
    void clear()
    {
        std::fill(array.begin(), array.end(), INVALID_ELEMENT);
    }

    /**
     * Get the underlying arrays size.
     *
     * \return size of the array.
     */
    size_type size()
    {
        return array.size();
    }

 protected:
    std::array<std::atomic<T>, SIZE> array;
};

/**
 * A specialization of the array_lock_free that allows removal of elements.
 */
template<typename T, size_t SIZE, T INVALID_ELEMENT = T()>
class array_lock_free_with_remove : public array_lock_free<T, SIZE, INVALID_ELEMENT>
{
    using parent_array = array_lock_free<T, SIZE, INVALID_ELEMENT>;

 public:
    array_lock_free_with_remove()
        : parent_array() {}

    /**
     * Remove the first occurrence of the given element from the array.
     *
     * \param value Value to be removed.
     *
     * \return True, if an element was removed. False otherwise.
     */
    bool remove(const T& value)
    {
        if (value == INVALID_ELEMENT) {
            return false;
        }

        auto remove_fn = [&value](std::atomic<T>& array_value) -> bool {
            auto tmp{ value };
            return array_value.compare_exchange_strong(tmp, INVALID_ELEMENT);
        };

        auto it{ std::find_if(parent_array::array.begin(), parent_array::array.end(), remove_fn) };

        return it != parent_array::array.end();
    }
};
