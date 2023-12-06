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

#pragma once

#include "csetjmp"
#include "cstdio"
#include "vector"

#include "assert.hpp"
#include "config.hpp"
#include "expect.hpp"

namespace baretest
{

    class test_suite;

    struct test_case
    {
        enum class result_t
        {
            SUCCESS,
            FAILURE,
            SKIPPED
        };
        using test_case_fn = result_t (*)();
        const char* name;
        test_case_fn fn_;

        test_case(test_suite& suite, const char* name, test_case_fn tc);
        bool run() const;
    };

    class test_suite
    {
     private:
        std::vector<test_case> test_cases;

     public:
        void add(test_case tc)
        {
            test_cases.push_back(tc);
        }
        void run();

        size_t get_test_count() const
        {
            return test_cases.size();
        }
    };

    jmp_buf& get_env();
    test_suite& get_suite();

}  // namespace baretest

#define TEST_CASE_CONDITIONAL(test_name, condition)                           \
    static baretest::test_case::result_t test_##test_name();                  \
    static void test_impl_##test_name();                                      \
    namespace baretest                                                        \
    {                                                                         \
        test_case tc_##test_name(get_suite(), #test_name, &test_##test_name); \
    }                                                                         \
    baretest::test_case::result_t test_##test_name()                          \
    {                                                                         \
        printf("test case: %s\n", __func__);                                  \
        if (not(condition)) {                                                 \
            printf("- skipping as condition is NOT met: `" #condition "`\n"); \
            return baretest::test_case::result_t::SKIPPED;                    \
        }                                                                     \
        int val = setjmp(baretest::get_env());                                \
        if (val) {                                                            \
            return baretest::test_case::result_t::FAILURE;                    \
        }                                                                     \
        test_impl_##test_name();                                              \
        return baretest::test_case::result_t::SUCCESS;                        \
    }                                                                         \
    void test_impl_##test_name()

#define TEST_CASE(test_name) TEST_CASE_CONDITIONAL(test_name, true)

/**
 * Print information about environment, such as the command line of the test.
 */
void print_environment_info();

void __attribute__((weak)) prologue();
void __attribute__((weak)) epilogue();

#define BENCHMARK_RESULT(name, value, unit) baretest::benchmark(name, value, unit)

#define BARETEST_RUN                 \
    int main()                       \
    {                                \
        print_environment_info();    \
        prologue();                  \
        baretest::get_suite().run(); \
        epilogue();                  \
        return 0;                    \
    }
