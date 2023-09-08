/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>

namespace cbl
{

   /** Interval class which models a discrete mathematical one dimensional interval.
 *
 * Intervals are denoted with [a, b), where a is the beginning point of
 * the interval, and b is the end point.
 * The endpoint is _not_ contained, as these intervals are half-open in the
 * mathematical sense.
 *
 * Intervals are iterable ranges.
 */
   template<typename T = size_t>
   struct interval_impl
   {
      using ival_t = T;

      ival_t a;  ///< The begin value
      ival_t b;  ///< The end value

      /// Constructs an interval which contains [begin, end)
      constexpr interval_impl(ival_t begin_, ival_t end_)
         : a{ begin_ }, b{ end_ } {}

      /// Constructs an empty interval
      constexpr interval_impl()
         : interval_impl<ival_t>(ival_t{}, ival_t{}) {}

      /// Constructs an interval from base and size
      static constexpr interval_impl<ival_t> from_size(ival_t base, ival_t size)
      {
         return { base, base + size };
      }

      /// Constructs an interval which only contains one point
      static constexpr interval_impl<ival_t> from_point(ival_t p)
      {
         return from_size(p, ival_t{ 1 });
      }

      /// Constructs an interval from base and power of 2 order for the size
      static constexpr interval_impl<ival_t> from_order(ival_t base, uint8_t order)
      {
         return from_size(base, ival_t{ 1ull } << order);
      }

      /// Tells if the interval is empty. Invalid intervals are also empty.
      constexpr bool empty() const
      {
         return b <= a;
      }

      /// Returns the size. The size of invalid intervals is 0.
      constexpr ival_t size() const
      {
         return empty() ? ival_t{ 0 } : b - a;
      }

      /// Tells if the interval intersects with another interval.
      constexpr bool intersects(const interval_impl<ival_t>& o) const
      {
         return not empty() and not o.empty() and a < o.b and o.a < b;
      }

      /// Returns the intersection of two intervals
      constexpr interval_impl<ival_t> intersection(const interval_impl<ival_t>& o) const
      {
         return interval_impl<ival_t>(std::max(a, o.a), std::min(b, o.b));
      }

      /// Returns if the given value is contained in this one
      constexpr bool contains(const ival_t& v) const
      {
         return not empty() and a <= v and b > v;
      }

      /// Returns if the given interval is contained in this one
      constexpr bool contains(const interval_impl<ival_t>& o) const
      {
         return not empty() and not o.empty() and a <= o.a and b >= o.b;
      }

      /// Returns true if given interval equals other interval exactly
      constexpr bool operator==(const interval_impl<ival_t>& o) const
      {
         return a == o.a && b == o.b;
      }

      /// Returns true if given interval does not equal the other interval
      constexpr bool operator!=(const interval_impl<ival_t>& o) const
      {
         return not this->operator==(o);
      }

      /// Returns true if interval ends before other interval begins
      ///
      /// Do not use on empty/invalid intervals.
      constexpr bool operator<(const interval_impl<ival_t>& o) const
      {
         return b <= o.a;
      }

      class iterator
      {
         ival_t pos;

      public:
         using difference_type = ptrdiff_t;
         using value_type = ival_t;
         using reference = ival_t&;
         using pointer = ival_t*;
         using iterator_category = std::bidirectional_iterator_tag;

         constexpr iterator(ival_t start)
            : pos{ start } {}

         constexpr ival_t operator*() const
         {
            return pos;
         }

         constexpr iterator& operator++()
         {
            ++pos;
            return *this;
         }

         constexpr iterator& operator+=(ival_t i)
         {
            pos += i;
            return *this;
         }

         constexpr ival_t operator-(const iterator& o) const
         {
            return pos - o.pos;
         }

         constexpr bool operator==(const iterator& o) const
         {
            return pos == o.pos;
         }

         constexpr bool operator!=(const iterator& o) const
         {
            return !(*this == o);
         }

         constexpr bool operator>(const iterator& o) const
         {
            return pos > o.pos;
         }
      };

      constexpr iterator begin() const
      {
         return { a };
      }
      constexpr iterator end() const
      {
         return { b };
      }
   };

   using interval = interval_impl<>;

   template<typename T = size_t>
   struct strided_interval_impl
   {
      using ival_t = T;

      interval_impl<ival_t> ival;
      ival_t stride;

      /**
     * Constructs a strided iterable interval from an already present interval.
     *
     * \param iv
     *        The interval that will be iterated over.
     * \param stride
     *        The stride in which to iterate. Cannot be zero and must evenly divide iv.
     */
      constexpr strided_interval_impl(const interval_impl<ival_t>& ival_, const ival_t stride_)
         : ival(ival_), stride(stride_)
      {
         assert(stride != ival_t{ 0 });
         assert(ival.size() % stride == ival_t{ 0 });
      }

      constexpr strided_interval_impl(const ival_t begin, const ival_t end, const ival_t stride_)
         : strided_interval_impl<ival_t>(interval_impl<ival_t>(begin, end), stride_)
      {}

      struct iterator
      {
         ival_t pos;
         ival_t stride;

         using difference_type = ssize_t;
         using value_type = ival_t;
         using reference = ival_t&;
         using pointer = ival_t*;
         using iterator_category = std::bidirectional_iterator_tag;

         constexpr iterator(ival_t pos_, ival_t stride_)
            : pos(pos_), stride(stride_) {}

         constexpr ival_t operator*() const
         {
            return pos;
         }

         constexpr iterator operator++(int)
         {
            auto back{ *this };
            this->operator++();
            return back;
         }

         constexpr iterator& operator++()
         {
            pos += stride;
            return *this;
         }

         constexpr iterator operator--(int)
         {
            auto back{ *this };
            this->operator--();
            return back;
         }

         constexpr iterator& operator--()
         {
            pos -= stride;
            return *this;
         }

         constexpr bool operator==(const iterator& o) const
         {
            return pos == o.pos;
         }

         constexpr bool operator!=(const iterator& o) const
         {
            return not operator==(o);
         }

         constexpr bool operator<(const iterator& o) const
         {
            return pos < o.pos;
         }
      };

      constexpr iterator begin() const
      {
         return { ival.a, stride };
      }
      constexpr iterator end() const
      {
         return { ival.b, stride };
      }
      constexpr std::reverse_iterator<iterator> rbegin() const
      {
         return std::make_reverse_iterator(end());
      }
      constexpr std::reverse_iterator<iterator> rend() const
      {
         return std::make_reverse_iterator(begin());
      }
   };

   using strided_interval = strided_interval_impl<>;

}  // namespace cbl
