// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <toyos/util/string.hpp>

TEST_CASE("split with empty input")
{
    CHECK(util::string::split("", ',').empty());
    CHECK(util::string::split("", '-').empty());
}

TEST_CASE("split without delimiter")
{
    CHECK(util::string::split("a", ',') == std::vector<std::string>{ "a" });
    CHECK(util::string::split("a", '-') == std::vector<std::string>{ "a" });
}

TEST_CASE("split with multiple elements")
{
    CHECK(util::string::split("a,b,c", ',') == std::vector<std::string>{ "a", "b", "c" });
    CHECK(util::string::split("a-b-c", '-') == std::vector<std::string>{ "a", "b", "c" });
}

TEST_CASE("split with empty elements in between")
{
    CHECK(util::string::split("a,b,,,c", ',') == std::vector<std::string>{ "a", "b", "", "", "c" });
    CHECK(util::string::split("a-b---c", '-') == std::vector<std::string>{ "a", "b", "", "", "c" });

    CHECK(util::string::split(", ,", ',') == std::vector<std::string>{ "", " ", "" });
    CHECK(util::string::split(",,", ',') == std::vector<std::string>{ "", "", "" });
    CHECK(util::string::split("a,", ',') == std::vector<std::string>{ "a", "" });
    CHECK(util::string::split(",b", ',') == std::vector<std::string>{ "", "b" });
}
