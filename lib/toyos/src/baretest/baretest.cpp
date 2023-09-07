/*
 * Copyright 2017 Cyberus Technology GmbH

 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <toyos/baretest/baretest.hpp>

void __attribute__((weak)) prologue()
{}
void __attribute__((weak)) epilogue()
{}

namespace baretest
{

jmp_buf& get_env()
{
    static jmp_buf env;
    return env;
}

test_suite& get_suite()
{
    static test_suite suite;
    return suite;
}

bool test_case::run() const
{
    result_t res = fn_();
    switch (res) {
    case result_t::SUCCESS: success(name); return true;
    case result_t::FAILURE: failure(name); return false;
    case result_t::SKIPPED:
    default: skip(); return false;
    }
}

test_case::test_case(test_suite& suite, const char* name_, test_case_fn tc) : name(name_), fn_(tc)
{
    suite.add(*this);
}

void test_suite::run()
{
    hello(test_cases.size());
    for (const auto& tc : test_cases) {
        tc.run();
    }
    goodbye();
}

__attribute__((noreturn)) void fail(const char* msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
    longjmp(baretest::get_env(), baretest::ASSERT_FAILED);
}

} // namespace baretest
