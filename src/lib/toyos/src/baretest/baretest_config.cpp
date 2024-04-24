// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <toyos/util/baretest_config.hpp>
#include <toyos/util/sotest.hpp>

namespace baretest
{
    void success(const char* name)
    {
        test_protocol::success(name);
    }
    void failure(const char* name)
    {
        test_protocol::fail(name);
    }
    void skip()
    {
        test_protocol::skip();
    }
    void hello(size_t test_count)
    {
        test_protocol::begin(test_count);
    }
    void goodbye()
    {
        test_protocol::end();
    }

    void benchmark(const char* name, long int value, const char* unit)
    {
        test_protocol::benchmark(name, value, unit);
    }

}  // namespace baretest
