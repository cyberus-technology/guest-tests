// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <type_traits>

namespace cbl
{

    template<typename T, bool = std::is_integral<T>::value>
    class in_place_atomic_base
    {
     protected:
        T* v_;

        bool compare_exchange_internal(T expected, T desired, std::memory_order success, std::memory_order failure, bool weak)
        {
            return __atomic_compare_exchange(v_, &expected, &desired, weak, success, failure);
        }

     public:
        explicit in_place_atomic_base(T& v)
            : v_(&v) {}

        bool compare_exchange_strong(T expected, T desired, std::memory_order success, std::memory_order failure)
        {
            return compare_exchange_internal(expected, desired, success, failure, false);
        }

        bool compare_exchange_weak(T expected, T desired, std::memory_order success, std::memory_order failure)
        {
            return compare_exchange_internal(expected, desired, success, failure, true);
        }

        T exchange(T desired, std::memory_order order = std::memory_order_seq_cst)
        {
            T ret;
            __atomic_exchange(this->v_, &desired, &ret, order);
            return ret;
        }

        bool is_lock_free()
        {
            return __atomic_is_lock_free(sizeof(T), v_);
        }

        using non_const_t = typename std::remove_const<T>::type;

        T load(std::memory_order order = std::memory_order_seq_cst) const
        {
            non_const_t ret;
            __atomic_load(v_, &ret, order);
            return ret;
        }

        void store(T desired, std::memory_order order = std::memory_order_seq_cst)
        {
            __atomic_store(v_, &desired, order);
        }
    };

    template<typename T>
    class in_place_atomic_base<T, true> : public in_place_atomic_base<T, false>
    {
        using base = in_place_atomic_base<T, false>;

     public:
        using base::base;

        T fetch_add(T val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_add(this->v_, val, order);
        }

        T fetch_and(T val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_and(this->v_, val, order);
        }

        T fetch_or(T val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_or(this->v_, val, order);
        }

        T fetch_sub(T val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_sub(this->v_, val, order);
        }

        T fetch_xor(T val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_xor(this->v_, val, order);
        }

        T operator++(int)
        {
            return fetch_add(1);
        }
        T operator--(int)
        {
            return fetch_sub(1);
        }

        T operator++()
        {
            return fetch_add(1) + 1;
        }
        T operator--()
        {
            return fetch_sub(1) - 1;
        }

        T operator+=(T v)
        {
            return fetch_add(v);
        }
        T operator-=(T v)
        {
            return fetch_sub(v);
        }
        T operator&=(T v)
        {
            return fetch_and(v);
        }
        T operator|=(T v)
        {
            return fetch_or(v);
        }
        T operator^=(T v)
        {
            return fetch_xor(v);
        }
    };

    template<typename T>
    class in_place_atomic : public in_place_atomic_base<T>
    {
        using base = in_place_atomic_base<T>;

     public:
        using base::base;

        T operator=(T desired)
        {
            base::store(desired);
            return desired;
        }
    };

    template<typename T>
    class in_place_atomic<T*> : public in_place_atomic_base<T*>
    {
        using base = in_place_atomic_base<T*>;

     public:
        using base::base;

        T* fetch_add(std::ptrdiff_t val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_add(this->v_, val * sizeof(T), order);
        }

        T* fetch_sub(std::ptrdiff_t val, std::memory_order order = std::memory_order_seq_cst)
        {
            return __atomic_fetch_sub(this->v_, val * sizeof(T), order);
        }

        T* operator++(int)
        {
            return fetch_add(1);
        }
        T* operator--(int)
        {
            return fetch_sub(1);
        }

        T* operator++()
        {
            return fetch_add(1) + 1;
        }
        T* operator--()
        {
            return fetch_sub(1) - 1;
        }

        T* operator+=(std::ptrdiff_t v)
        {
            return fetch_add(v);
        }
        T* operator-=(std::ptrdiff_t v)
        {
            return fetch_sub(v);
        }
    };

}  // namespace cbl
