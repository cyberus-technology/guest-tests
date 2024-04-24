// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "assert.hpp"
#include "print.hpp"

namespace baretest
{

    template<typename T>
    class expectation
    {
        const T expected_value;

        void print_result(bool result, const T& other) const
        {
            if (not result) {
                printf("Expect: Mismatch: expected: ");
                print(expected_value);
                printf("Actual: ");
                print(other);
                printf(" \n");
            }
        }

     public:
        expectation(const T& expect)
            : expected_value(expect) {}

        bool operator==(const T& other)
        {
            bool result = (expected_value == other);
            print_result(result, other);
            return result;
        }

        bool operator!=(const T& other)
        {
            bool result = not(expected_value == other);
            print_result(result, other);
            return result;
        }

        bool operator>(const T& other)
        {
            bool result = (expected_value > other);
            print_result(result, other);
            return result;
        }

        bool operator>=(const T& other)
        {
            bool result = (expected_value >= other);
            print_result(result, other);
            return result;
        }

        bool operator<(const T& other)
        {
            bool result = (expected_value < other);
            print_result(result, other);
            return result;
        }

        bool operator<=(const T& other)
        {
            bool result = (expected_value <= other);
            print_result(result, other);
            return result;
        }
    };

}  // namespace baretest

#define BARETEST_VERIFY(stmt)                                                         \
    do {                                                                              \
        if (not(stmt)) {                                                              \
            baretest::fail("Expect: %s failed @ %s:%d\n", #stmt, __FILE__, __LINE__); \
        }                                                                             \
    } while (false);
