/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

// This test tests functional aspects of SLVM log exposition.

#include <arch.hpp>
#include <baretest/baretest.hpp>
#include <slvm/api.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

TEST_CASE(slvm_vmcall_log_is_available)
{
    BARETEST_ASSERT(slvm_vmcall_log_is_present());
}

TEST_CASE(slvm_vmcall_read_vmm_log_with_zero_size_fails)
{
    static std::array<char, PAGE_SIZE> log {};

    auto log_size {slvm_vmcall_log_read_vmm(ptr_to_num(log.data()), 0)};
    BARETEST_ASSERT(log_size == SLVMAPI_VMCALL_EFAULT);
}

TEST_CASE(slvm_vmcall_read_vmm_log_to_invalid_address_fails)
{
    auto log_size {slvm_vmcall_log_read_vmm(0, PAGE_SIZE)};
    BARETEST_ASSERT(log_size == SLVMAPI_VMCALL_EFAULT);
}

TEST_CASE(slvm_vmcall_read_vmm_log_contains_first_message)
{
    static constexpr std::string_view FIRST_MESSAGE {"memlogger has been enabled"};

    // We must make sure not to consume the entire log in order for the succeeding tests to work correctly.
    static std::array<char, 128> log {};

    auto log_size {slvm_vmcall_log_read_vmm(ptr_to_num(log.data()), log.size())};

    BARETEST_ASSERT(log_size > 0);

    // Log message have the format [<level> <source>] <message>.
    // To check for the expected string, we have to find the first ']' character,
    // skip it and the subsequent space.
    auto end_of_prefix {std::find(log.begin(), log.end(), ']')};
    BARETEST_ASSERT(end_of_prefix != log.end());
    BARETEST_ASSERT(std::equal(FIRST_MESSAGE.begin(), FIRST_MESSAGE.end(), std::next(end_of_prefix, 2)));
}

TEST_CASE(slvm_vmcall_read_vmm_log_does_not_write_outside_array_boundaries)
{
    // This test assumes that there is still at least one message in the log.

    static constexpr uint8_t FILL_PATTERN {0xAA};

    alignas(PAGE_SIZE) static std::array<uint8_t, PAGE_SIZE> log {};
    log.fill(FILL_PATTERN);

    auto log_size {slvm_vmcall_log_read_vmm(ptr_to_num(log.data() + PAGE_SIZE / 2), log.size() / 2)};

    BARETEST_ASSERT(log_size > 0 and log_size <= static_cast<int>(log.size()));

    BARETEST_ASSERT(
        std::all_of(log.begin(), log.begin() + PAGE_SIZE / 2, [](uint8_t val) { return val == FILL_PATTERN; }))

    BARETEST_ASSERT(
        std::all_of(log.begin() + PAGE_SIZE / 2 + log_size, log.end(), [](uint8_t val) { return val == FILL_PATTERN; }))
}

TEST_CASE(slvm_vmcall_read_vmm_log_size_smaller_than_a_page_succeeds)
{
    alignas(PAGE_SIZE) static std::array<char, 1> log {};

    auto log_size {slvm_vmcall_log_read_vmm(ptr_to_num(log.data()), log.size())};
    BARETEST_ASSERT(log_size >= 0 and log_size <= static_cast<int>(log.size()));
}

BARETEST_RUN;
