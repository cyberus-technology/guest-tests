/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <initializer_list>
#include <tuple>

#include "helpers.hpp"

#include <baretest/baretest.hpp>
#include <compiler.hpp>
#include <logger/trace.hpp>

TEST_CASE(mov_from_segment)
{
    for (auto f : {mov_from_cs, mov_from_ds, mov_from_es, mov_from_fs, mov_from_gs, mov_from_ss}) {
        baretest::expectation<uint16_t> sel_native(f(false));
        uint16_t sel_emul {f(true)};
        BARETEST_VERIFY(sel_native == sel_emul);
    }
}

TEST_CASE(mov_to_segment)
{
    std::initializer_list<std::pair<decltype(&mov_from_ds), decltype(&mov_to_ds)>> segment_pairs {
        {&mov_from_ds, &mov_to_ds}, {&mov_from_es, &mov_to_es}, {&mov_from_fs, &mov_to_fs},
        {&mov_from_gs, &mov_to_gs}, {&mov_from_ss, &mov_to_ss},
    };

    baretest::expectation<uint16_t> data(SEL_DATA);
    for (auto p : segment_pairs) {
        p.second(SEL_DATA, true);
        auto sel = p.first(false);
        BARETEST_VERIFY(data == sel);
    }
}
