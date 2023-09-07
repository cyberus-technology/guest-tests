/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <array>
#include <functional>
#include <numeric>
#include <type_traits>

#include "math.hpp"
#include <toyos/util/interval.hpp>
#include <toyos/util/traits.hpp>

namespace cbl
{

   using phys_t = uint64_t;
   using math::order_t;

   template<size_t N>
   class order_range_
   {
      static_assert(N > 0, "This data structure makes no sense for N<1");
      using bases_t = std::array<phys_t, N>;
      bases_t base;
      phys_t size;
      order_t max_order;

   public:
      constexpr order_range_(const bases_t& bl, phys_t size_, order_t max_order_ = order_t(~0ull))
         : base{ bl }, size{ size_ }, max_order{ max_order_ }
      {}

      class iterator
      {
         bases_t base;
         phys_t rest_size;
         order_t max_order_choice;
         phys_t current_step;

         static size_t order_step(phys_t base, size_t rest, order_t max_order_user)
         {
            const phys_t max_bit{ 1ull << (bit_width<size_t>() - 1) };
            const order_t min_order(math::order_min(base | max_bit));
            const order_t max_order{ std::min(order_t(math::order_max(rest)), max_order_user) };
            const size_t step{ 1ull << std::min(min_order, max_order) };
            return step;
         }

         phys_t update_step()
         {
            if(!rest_size) {
               return 0;
            }
            const phys_t mix_base{ std::accumulate(std::begin(base), std::end(base), size_t(0), std::bit_or<size_t>()) };
            return current_step = order_step(mix_base, rest_size, max_order_choice);
         }

      public:
         using iterator_category = std::forward_iterator_tag;
         using value_type = std::conditional_t<N == 1, interval, std::pair<interval, interval>>;
         using difference_type = std::ptrdiff_t;
         using pointer = value_type*;
         using reference = value_type&;

         iterator(const bases_t& bs, phys_t size, order_t mo)
            : base{ bs }, rest_size{ size }, max_order_choice{ mo }, current_step{ update_step() }
         {}

         interval get_ivals(uint2type<1>) const
         {
            return { base[0], base[0] + current_step };
         }

         std::pair<interval, interval> get_ivals(uint2type<2>) const
         {
            return { { base[0], base[0] + current_step }, { base[1], base[1] + current_step } };
         }

         auto operator*() const
         {
            return get_ivals(uint2type<N>{});
         }

         // operator* can be implemented for N>2 if needed.

         iterator& operator++()
         {
            for(auto& b : base) {
               b += current_step;
            }
            rest_size -= current_step;
            update_step();
            return *this;
         }

         bool operator==(const iterator& o) const
         {
            const bool eq{ std::equal(std::begin(base), std::end(base), std::begin(o.base), std::end(o.base)) };
            return eq && rest_size == o.rest_size;
         }

         bool operator!=(const iterator& o) const
         {
            return !(*this == o);
         }
      };

      iterator begin() const
      {
         return { base, size, max_order };
      }
      iterator end() const
      {
         auto bs(base);
         for(auto& b : bs) {
            b += size;
         }
         return { bs, 0, max_order };
      }
   };

   class order_range : public order_range_<1>
   {
   public:
      constexpr order_range(phys_t base_, phys_t size_, order_t max_order_ = order_t(~0ull))
         : order_range_({ { base_ } }, size_, max_order_)
      {}

      constexpr order_range(const interval& i, order_t max_order_ = order_t(~0ull))
         : order_range_({ { i.a } }, i.size(), max_order_)
      {}
   };

   class order_range2 : public order_range_<2>
   {
   public:
      constexpr order_range2(phys_t base1_, phys_t base2_, phys_t size_, order_t max_order_ = order_t(~0ull))
         : order_range_({ { base1_, base2_ } }, size_, max_order_)
      {}
   };

}  // namespace cbl
