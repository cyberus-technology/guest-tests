/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <type_traits>

#include <cstdint>
/**
 * \brief number to pointer conversion
 * \param number to convert
 * \return converted pointer value
 *
 * This function converts a given number to its pointer representant
 *
 */
template<class T, class V>
static constexpr inline T* num_to_ptr(V num)
{
   return reinterpret_cast<T*>(static_cast<uintptr_t>(num));
}

/**
 * \brief pointer to number conversion
 * \param pointer to convert
 * \return converted number value
 *
 * This function converts a given pointer to its number representant
 */
template<class T>
static constexpr inline uintptr_t ptr_to_num(T* ptr)
{
   return uintptr_t(ptr);
}

/**
 * \brief converts value of enum type to underlying atomic type value
 * \param enum type to convert
 * \return underlying type
 *
 * This function converts a value of enum's type to its underlying type value
 *
 */
template<typename T>
static constexpr auto to_underlying(const T& e)
{
   return static_cast<std::underlying_type_t<T>>(e);
}
