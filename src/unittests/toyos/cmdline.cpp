// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <toyos/cmdline.hpp>

TEST_CASE("parsing empty cmdline only results in defaults")
{
    auto input = "";
    auto parsed = cmdline::cmdline_parser(input);
    CHECK(!parsed.serial_option().has_value());
    CHECK(!parsed.xhci_option().has_value());
    CHECK(parsed.xhci_power_option() == "0");  // default
    CHECK(parsed.disable_testcases_option().empty());
}

TEST_CASE("parsing '--serial'")
{
    auto input = "--serial";
    auto parsed = cmdline::cmdline_parser(input);
    CHECK(parsed.serial_option().value().empty());

    input = "--serial=0x3f8";
    parsed = cmdline::cmdline_parser(input);
    CHECK(parsed.serial_option().value() == "0x3f8");
}

TEST_CASE("parsing '--disable-testcases'")
{
    auto input = "--disable-testcases=testA,testB,testC";
    auto parsed = cmdline::cmdline_parser(input);
    auto disable_tests = parsed.disable_testcases_option();
    CHECK(disable_tests[0] == "testA");
    CHECK(disable_tests[1] == "testB");
    CHECK(disable_tests[2] == "testC");
}
