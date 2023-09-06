/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <map>
#include <optional>
#include <logger/trace.hpp>
#include <vector>

namespace cbl
{

/** Kind of std::map clone, but based on std::vector
 *
 * This class provides the same semantics as an std::map regarding insert and lookup,
 * but works on an std::vector.
 *
 * This data structure is optimized for reading. Inserting elements is expensive.
 */
template <typename key_t, typename value_t>
class map_vector
{
public:
    using pair_t = std::pair<key_t, value_t>;
    using iterator = typename std::vector<pair_t>::iterator;
    using const_iterator = typename std::vector<pair_t>::const_iterator;

    std::vector<std::pair<key_t, value_t>> vec;

    static bool pair_comp(const pair_t& a, const pair_t& b) { return a.first < b.first; }

    static bool pair_comp_key(const pair_t& a, const key_t& b) { return a.first < b; }

public:
    map_vector() = default;

    /** Insert an item at specific position
     *
     * Insertion hint is checked. If insertion hint is invalid, O(log(N))
     * lookup for correct positionwill be started.
     *
     * \param it Iterator which points to the position at which the inserted item shall be inserted
     * \param key The key by which the item shall be looked up with later
     * \param val The payload value which is associated with the key
     *
     * \return Iterator which points to the inserted item, wrapped into an optional.
     *         Invalid if insertion failed.
     */
    std::optional<iterator> insert(const_iterator it, key_t key, value_t val)
    {
        if (it != end() && !(key < it->first)) {
            std::cout << "BAD HINT! (1: " << key << " !< " << it->first << ")\n";
            return insert(key, val);
        }
        if (it != begin() && !(std::prev(it, 1)->first < key)) {
            std::cout << "BAD HINT! (2)\n";
            return insert(key, val);
        }

        auto pos(vec.emplace(it, std::make_pair(std::move(key), std::move(val))));
        return {pos};
    }

    /** Insertion of key-value pair with automatic position lookup
     *
     * \param key The key by which the item shall be looked up with later
     * \param val The payload value which is associated with the key
     *
     * \return Iterator which points to the inserted item, wrapped into an optional.
     *         Invalid if insertion failed.
     */
    std::optional<iterator> insert(key_t key, value_t val)
    {
        const auto pos(std::lower_bound(std::begin(vec), std::end(vec), key, pair_comp_key));
        if (pos == std::end(vec) || pos->first != key) {
            auto it(vec.emplace(pos, std::make_pair(std::move(key), std::move(val))));
            return {it};
        }
        return {};
    }

    /** Find value for key
     *
     * O(log(N)) lookup.
     *
     * \param key Key to lookup
     *
     * \return Iterator to key-value pair. End-iterator, if item could not be found.
     */
    iterator find(const key_t& k) const
    {
        const auto rng(std::find(std::begin(vec), std::end(vec), k, pair_comp_key));
        if (rng.first == rng.second) {
            return rng.first;
        }
        return std::end(vec);
    }

    /** Lower bound search.
     *
     * Returns iterator to first item with (item.key >= key)
     *
     * \param key Key to lookup
     *
     * \return Iterator to lower bound position
     */
    auto lower_bound(const key_t& k)
    {
        const auto it(std::lower_bound(std::begin(vec), std::end(vec), k, pair_comp_key));
        return it;
    }

    /** Erase items
     *
     * \param a begin iterator of erasure range
     * \param b end iterator of erasure range
     */
    void erase(iterator a, iterator b) { vec.erase(a, b); }

    /** Reserve heap memory backing storage for specific future size
     *
     * \param cap Number of items to reserve space for
     */
    void reserve(size_t cap) { vec.reserve(cap); }

    /** Return number of items which are currently stored */
    size_t size() const { return vec.size(); }

    iterator begin() { return std::begin(vec); }
    iterator end() { return std::end(vec); }

    const_iterator begin() const { return std::begin(vec); }
    const_iterator end() const { return std::end(vec); }

    const_iterator cbegin() const { return std::cbegin(vec); }
    const_iterator cend() const { return std::cend(vec); }
};

/** std::map_vector compatible std::map frontend
 *
 * This class provides the same semantics as an std::map regarding insert and lookup,
 * but works on an std::vector. This is much faster when inserting seldom, and looking up often.
 */
template <typename key_t, typename value_t>
class map_map
{
public:
    using map_t = std::map<key_t, value_t>;
    using iterator = typename map_t::iterator;
    using const_iterator = typename std::map<key_t, value_t>::const_iterator;

    std::map<key_t, value_t> map;

public:
    map_map() = default;

    /** Insert an item at specific position
     *
     * Insertion hint is checked. If insertion hint is invalid, O(log(N))
     * lookup for correct positionwill be started.
     *
     * \param it Iterator which points to the position at which the inserted item shall be inserted
     * \param key The key by which the item shall be looked up with later
     * \param val The payload value which is associated with the key
     *
     * \return Iterator which points to the inserted item, wrapped into an optional.
     *         Invalid if insertion failed.
     */
    std::optional<iterator> insert(const_iterator it, key_t key, value_t val)
    {
        const auto ret(map.emplace_hint(it, std::make_pair(std::move(key), std::move(val))));
        if (ret != end()) {
            return {ret};
        }
        return {};
    }

    /** Insertion of key-value pair with automatic position lookup
     *
     * \param key The key by which the item shall be looked up with later
     * \param val The payload value which is associated with the key
     *
     * \return Iterator which points to the inserted item, wrapped into an optional.
     *         Invalid if insertion failed.
     */
    std::optional<iterator> insert(key_t key, value_t val)
    {
        const auto ret(map.emplace(std::make_pair(std::move(key), std::move(val))));
        if (ret.second) {
            return {ret.first};
        }
        return {};
    }

    /** Find value for key
     *
     * O(log(N)) lookup.
     *
     * \param key Key to lookup
     *
     * \return Iterator to key-value pair. End-iterator, if item could not be found.
     */
    iterator find(const key_t& k) const { return map.find(k); }

    /** Lower bound search.
     *
     * Returns iterator to first item with (item.key >= key)
     *
     * \param key Key to lookup
     *
     * \return Iterator to lower bound position
     */
    auto lower_bound(const key_t& k) { return map.lower_bound(k); }

    /** Reserve heap memory backing storage for specific future size
     *
     * \param cap Number of items to reserve space for
     */
    void reserve(size_t /*cap*/)
    { /* NOP */
    }

    /** Return number of items which are currently stored */
    size_t size() const { return map.size(); }

    iterator begin() { return std::begin(map); }
    iterator end() { return std::end(map); }

    const_iterator begin() const { return std::begin(map); }
    const_iterator end() const { return std::end(map); }

    const_iterator cbegin() const { return std::cbegin(map); }
    const_iterator cend() const { return std::cend(map); }

    auto& internal_map() { return map; }
};

} // namespace cbl
