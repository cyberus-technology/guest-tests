// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <toyos/util/cpuid.hpp>

/**
 * We don't want to restrict this test to a specific CPU. Hence, we just test
 * that it is a valid string and in a certain range.
 */
TEST_CASE("cpuid-extended-brand-name")
{
    auto name = util::cpuid::get_extended_brand_string();
    CHECK(!name.empty());
    CHECK(name.length() <= 48);
}
