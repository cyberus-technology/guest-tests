// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <toyos/baretest/baretest.hpp>
#include <toyos/boot.hpp>
#include <toyos/boot_cmdline.hpp>
#include <toyos/cmdline.hpp>
#include <toyos/util/cpuid.hpp>

void __attribute__((weak)) prologue()
{}
void __attribute__((weak)) epilogue()
{}

void print_environment_info()
{
    printf("Running Guest Test\n");
    printf("  load addr : %#x\n", load_addr());
    printf("  boot      : %s\n", boot_method_name(current_boot_method.value()));
    printf("  cmdline   : %s\n", get_boot_cmdline().value_or("").c_str());
    printf("  cpu vendor: %s\n", util::cpuid::get_vendor_id().c_str());
    printf("  cpu       : %s\n", util::cpuid::get_extended_brand_string().c_str());
    printf("              ");
    if (util::cpuid::hv_bit_present()) {
        printf("Hypervisor bit set\n");
    }
    else {
        printf("Hypervisor bit not set\n");
    }
    printf("\n");
};

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
            case result_t::SUCCESS:
                success(name);
                return true;
            case result_t::FAILURE:
                failure(name);
                return false;
            case result_t::SKIPPED:
                skip();
                return false;
            default:
                PANIC("Unexpected test case result: %#x", res);
        }
    }

    test_case::test_case(test_suite& suite, const char* name_, test_case_fn tc)
        : name(name_), fn_(tc)
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

    bool testcase_disabled_by_cmdline(const std::string_view& name)
    {
        auto cmdline = get_boot_cmdline().value_or("");
        auto parser = cmdline::cmdline_parser(cmdline);
        auto disabled = parser.disable_testcases_option();
        auto name_found = std::find(disabled.begin(), disabled.end(), name) != disabled.end();
        auto name_with_test_prefix_found = std::find(disabled.begin(), disabled.end(), std::string("test_") + std::string(name)) != disabled.end();
        return name_found || name_with_test_prefix_found;
    }

}  // namespace baretest
