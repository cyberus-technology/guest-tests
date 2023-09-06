/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <baretest/baretest_config.hpp>
#include <sotest/sotest.hpp>

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

} // namespace baretest
