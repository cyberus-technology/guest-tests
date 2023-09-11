/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include "algorithm"
#include "string"
#include "vector"

template<typename Range, typename Pred, typename T>
T find_if_or(const Range& r, Pred p, T alternative)
{
   const auto m(std::find_if(begin(r), end(r), p));
   return m != end(r) ? *m : alternative;
}

// This function tokenizes a given string using the specified token. It's a dumb version for now,
// but can be easily optimized later without changing the code that uses it.
template<typename T>
inline std::vector<T> tokenize(const T& input, const typename T::value_type token)
{
   T arg;
   std::vector<T> ret;

   for (const auto& c : input) {
      if (c == token) {
         if (arg.find_first_not_of(token) != T::npos) {
            ret.push_back(arg);
         }
         arg.clear();
         continue;
      }
      arg.push_back(c);
   }
   if (arg.find_first_not_of(token) != T::npos) {
      ret.push_back(arg);
   }

   return ret;
}

inline std::vector<std::string> tokenize(const char* input, char token)
{
   return tokenize(std::string(input), token);
}

inline std::vector<std::u16string> tokenize(const char16_t* input, char16_t token)
{
   return tokenize(std::u16string(input), token);
}

// Performance optimization: move the element
// to the last position and remove it then,
// thus, avoiding copies on larger vectors.
// Returns true if an element was found and erased.
template<typename T>
bool swap_erase(std::vector<T>& vec, const typename std::vector<T>::iterator& it)
{
   if (it != vec.end()) {
      std::iter_swap(it, vec.end() - 1);
      vec.erase(vec.end() - 1);
      return true;
   }
   return false;
}

// This function takes an element and a list of elements of the same type and checks if the given single
// element is in the given list.
template<typename T>
bool is_in(const T& elem, std::initializer_list<T> list)
{
   return std::find(std::begin(list), std::end(list), elem) != std::end(list);
}
