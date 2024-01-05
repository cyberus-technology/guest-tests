/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <string>
#include <vector>

namespace util::string
{
    /**
     * Splits a string. For example: ("a,b,c", ',') == ["a", "b", "c"].
     *
     * If the input is empty, the returned vector is empty. If the input is not
     * empty but the delimiter not found, the returned vector contains the
     * input.
     */
    std::vector<std::string> split(const std::string& source, char delimiter);
}  // namespace util::string
