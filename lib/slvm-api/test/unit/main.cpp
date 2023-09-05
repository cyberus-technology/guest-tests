/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <gtest/gtest.h>

#include <hedron/mtd.hpp>
#include <hedron/utcb_layout.hpp>

/*
 * This test checks if the headers of the library hedron-api are standalone
 * so the actual test does not matter in any case
 */

using segment_descriptor = hedron::segment_descriptor;

TEST(hedron_api, headers_are_standalone)
{
    hedron::segment_descriptor desc;
    desc.sel = 0xde;
    desc.ar = segment_descriptor::SEGMENT_AR_L | segment_descriptor::SEGMENT_AR_G;
    ASSERT_EQ(desc.ar, segment_descriptor::SEGMENT_AR_L | segment_descriptor::SEGMENT_AR_G);
    ASSERT_EQ(desc.sel, 0xde);
}
