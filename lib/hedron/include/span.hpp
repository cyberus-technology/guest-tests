/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <stdio.h>

template <typename T>
class span
{
    /** begin of the array */
    T* begin_;

    /** number of elements */
    size_t size_;

    using iterator = T*;
    using citerator = const T*;

public:
    span(T* b, size_t s) : begin_(b), size_(s) {}

    iterator begin() { return begin_; }
    citerator begin() const { return begin_; }

    iterator end() { return begin_ + size_; }

    citerator end() const { return begin_ + size_; }

    size_t size() const { return size_; }

    T& operator[](size_t pos) { return begin_[pos]; }
    const T& operator[](size_t pos) const { return begin_[pos]; }
};
