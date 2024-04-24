// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "interval.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <vector>

namespace cbl
{

    /** Associative data structure which maps numeric intervals to values
 *
 * Same functionality and signature as cbl::interval_map
 *
 * Stores items in consecutive memory for faster lookup.
 * (Tradeoff vs. slower construction)
 *
 * This structure cannot be empty.
 * It is considered empty if it contains one single range which fills
 * the whole numeric key interval range, and maps that to a default value.
 */
    template<typename key_t, typename value_t>
    class interval_vector_impl
    {
     private:
        using ival_t = key_t;
        using this_t = interval_vector_impl<ival_t, value_t>;
        using pair_t = std::pair<ival_t, value_t>;
        std::vector<pair_t> m;

        static constexpr ival_t ival_last{ ival_t(~0ull) };

        using it_t = typename std::vector<pair_t>::iterator;
        using cit_t = typename std::vector<pair_t>::const_iterator;

        auto lower_bound(ival_t point) const
        {
            auto comp([](const pair_t& a, const ival_t& b) { return a.first < b; });
            return std::lower_bound(std::begin(m), std::end(m), point, comp);
        }

        auto lower_bound(ival_t point)
        {
            auto comp([](const pair_t& a, const ival_t& b) { return a.first < b; });
            return std::lower_bound(std::begin(m), std::end(m), point, comp);
        }

        auto upper_bound(ival_t point) const
        {
            auto comp([](const ival_t& a, const pair_t& b) { return a < b.first; });
            return std::upper_bound(std::begin(m), std::end(m), point, comp);
        }

        auto upper_bound(ival_t point)
        {
            auto comp([](const ival_t& a, const pair_t& b) { return a < b.first; });
            return std::upper_bound(std::begin(m), std::end(m), point, comp);
        }

        template<typename ITER>
        void merge(ITER new_elem)
        {
            auto cur_elem{ new_elem };

            if (cur_elem == std::end(m)) {
                return;
            }

            if (const auto pre_elem{ std::prev(new_elem) };
                cur_elem != std::begin(m) and cur_elem->second == pre_elem->second) {
                // merge with predecessor
                cur_elem->first = pre_elem->first;
                cur_elem = m.erase(pre_elem);
            }

            if (const auto post_elem{ std::next(cur_elem) };
                post_elem != std::end(m) and cur_elem->second == post_elem->second) {
                // merge with successor
                m.erase(post_elem);
            }
        }

     public:
        using mod_ival_pair = std::pair<interval_impl<ival_t>, value_t&>;
        using ival_pair = std::pair<interval_impl<ival_t>, const value_t&>;

        interval_vector_impl(value_t val = value_t{})
            : m{ pair_t{ ival_t{}, val } } {}

        /** Lookup range
     *
     * \param point A point which lies within some interval
     *
     * \return Optional pair of cbl::interval which was found and the value associated to it
     */
        std::optional<ival_pair> find(ival_t point) const
        {
            cit_t upper{ upper_bound(point) };
            cit_t lower{ std::prev(upper) };
            const ival_t upper_val{ upper == std::end(m) ? ival_last : upper->first };
            const interval_impl<ival_t> ival{ lower->first, upper_val };

            return { ival_pair(ival, lower->second) };
        }

        std::optional<mod_ival_pair> find(ival_t point)
        {
            auto cr{ static_cast<const this_t&>(*this).find(point) };
            if (!cr) {
                return {};
            }
            return { mod_ival_pair{ cr->first, const_cast<value_t&>(cr->second) } };
        }

        struct interval_range
        {
            interval_range(const cit_t& b, const cit_t& e)
                : begin_(b), end_(e) {}

            const cit_t begin_;
            const cit_t end_;

            cit_t begin() const
            {
                return begin_;
            }
            cit_t end() const
            {
                return end_;
            }
        };

        /**
     * Returns an iterator that can be used to iterate over a range starting at
     * the interval containing the begin_point and ending at the interval
     * containing the end_point.
     */
        interval_range iterate_range(ival_t begin_point, ival_t end_point) const
        {
            auto begin_{ lower_bound(begin_point) };
            if (begin_->first != begin_point) {
                begin_ = std::prev(begin_);
            }

            auto end_{ lower_bound(end_point) };
            return interval_range(begin_, end_);
        }

        template<typename T>
        interval_range iterate_range(const T& range) const
        {
            return iterate_range(range.a, range.b);
        }

        /** Destructive insert. Overwrites existing intervals in case of overlap.
     */
        bool insert(const interval_impl<ival_t>& ival, const value_t& val)
        {
            assert(!ival.empty());
            assert(std::begin(m) != std::end(m));

            const auto lba{ lower_bound(ival.a) };  // first elem greater or equal ival.a
            const auto lbb{ lower_bound(ival.b) };  // first elem greater or equal ival.b

            if (lba == lbb) {  // we insert something inside another interval
                assert(lba != std::begin(m));

                const auto interv_to_insert{ std::prev(lbb) };

                if (val == interv_to_insert->second) {  // pointless insert with same value
                    return true;
                }

                if (ival.b == ival_last) {  // interval to the end, just insert one point
                    m.insert(lbb, pair_t{ ival.a, val });
                    return true;
                }

                if (lbb == std::end(m) or ival.b != lbb->first) {  // we have to insert two points at ival.a and ival.b
                    const auto cur_val{ std::prev(lba)->second };
                    const auto following{ m.insert(lbb, pair_t{ ival.a, val }) };

                    m.insert(std::next(following), pair_t{ ival.b, cur_val });
                    return true;
                }

                const auto new_elem{ m.insert(lbb, pair_t{ ival.a, val }) };
                merge(new_elem);

                return true;
            }

            if (lbb == std::end(m)) {  // we insert something at the end

                if (ival.b == ival_last) {  // we insert up to the end, so delete everything up to the end
                    m.erase(lba, lbb);
                    const auto inserted{ m.insert(std::end(m), pair_t{ ival.a, val }) };
                    merge(inserted);
                    return true;
                }

                auto prev{ std::prev(lbb) };
                prev = m.erase(lba, prev);

                if (prev->second == val) {  // values are the same, just shift following interval to the ival.a
                    prev->first = ival.a;
                    merge(prev);
                }
                else {  // shift following interval to ival.b and insert new one before
                    prev->first = ival.b;
                    const auto inserted{ m.insert(prev, pair_t{ ival.a, val }) };
                    merge(inserted);
                }

                return true;
            }

            auto delete_up_to{ lbb };
            auto prev_lbb{ std::prev(lbb) };

            if (ival.b != lbb->first) {  // if we do not end exactly on lbb, we need to shift prev_lbb forward
                prev_lbb->first = ival.b;
                delete_up_to = prev_lbb;
            }

            const auto to_insert{ m.erase(lba, delete_up_to) };
            const auto new_elem{ m.insert(to_insert, pair_t{ ival.a, val }) };

            merge(new_elem);

            return true;
        }

        /** Remove interval which contains point
     */
        bool remove(ival_t point)
        {
            const it_t next{ upper_bound(point) };
            const it_t actual{ std::prev(next) };

            if (next != std::end(m)) {
                if (actual == std::begin(m)) {
                    next->first = 0;
                }
                else {
                    const it_t before{ std::prev(actual) };
                    if (before->second == next->second) {
                        m.erase(next);
                    }
                }
            }
            else if (actual == std::begin(m)) {  // we have just one interval
                actual->second = value_t{};
                return true;
            }

            m.erase(actual);

            return true;
        }

        auto begin() const
        {
            return std::begin(m);
        }
        auto end() const
        {
            return std::end(m);
        }

        auto& internal_map()
        {
            return m;
        }
        const auto& internal_map() const
        {
            return m;
        }

        /**
     * Finds an interval of the given size contained in this map.
     *
     * \param size  The size of the desired interval.
     * \param key   The key to restrict the search to.
     * \param limit A value below which the entire resulting interval has to be.
     *              This value must not be contained in the resulting interval.
     * \return The highest interval of the given size fulfilling the constraints, or an empty one.
     */
        interval_impl<ival_t> find_last_fit(size_t size, const value_t& key, ival_t limit = ival_last) const
        {
            interval_impl<ival_t> final_range;

            for (const auto& elem : *this) {
                if (elem.second == key) {
                    // We do a self-search here because the current iterator does only
                    // provide the beginning of the region and not the end which we need
                    // to calculate the region size. This is a workaround!
                    // The proper solution would be to change the iterator (and all
                    // places where it is currently used) in a way so that one can
                    // access the entire interval during iteration.
                    auto pair{ find(elem.first) };
                    const interval_impl<ival_t>& candidate{ pair->first };
                    if (candidate.size() >= size and candidate.a + size <= limit) {
                        ival_t end = std::min(candidate.b, limit);
                        final_range = { end - size, end };
                    }
                }
            }

            return final_range;
        }
    };

    template<typename value_t>
    using interval_vector = interval_vector_impl<size_t, value_t>;

}  // namespace cbl
