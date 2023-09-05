/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <memory>

#include <compiler.h>

namespace cbl
{

/** Trait for determining the type of the parameter of a one-param function.
 *
 * Usage: decltype(cbl::func_param_type(&my_function))
 */
template <typename R, typename P>
static constexpr P func_param_type(R (*)(P))
{
    __UNREACHED__;
}

template <typename T>
static constexpr size_t bit_width(T = T {})
{
    return sizeof(T) * 8;
}

template <size_t N>
struct uint2type
{};

template <class T>
struct is_smart_ptr : std::false_type
{};

template <class T, class D>
struct is_smart_ptr<std::unique_ptr<T, D>> : std::true_type
{};

template <class... T>
struct is_smart_ptr<std::shared_ptr<T...>> : std::true_type
{};

template <typename T>
static constexpr bool is_smart_ptr_v {is_smart_ptr<T>::value};

template <typename T, size_t N>
static constexpr size_t array_items(const T (&)[N] = {})
{
    return N;
}

} // namespace cbl
