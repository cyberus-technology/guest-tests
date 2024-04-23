// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <catch2/catch_test_macros.hpp>

#include <toyos/console/console_serial_util.hpp>

TEST_CASE("get_effective_serial_port() parses decimal input")
{
    CHECK(get_effective_serial_port("42", nullptr) == 42);
}

TEST_CASE("get_effective_serial_port() parses hex input")
{
    CHECK(get_effective_serial_port("0x38f", nullptr) == 0x38f);
}
