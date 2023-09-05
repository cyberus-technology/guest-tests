/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <gtest/gtest.h>

#include <hedron/cap_pool.hpp>
#include <hedron/cap_range.hpp>
#include <hedron/crd.hpp>
#include <hedron/delay.hpp>
#include <hedron/ec.hpp>
#include <hedron/event_selectors.hpp>
#include <hedron/generic_cap_range.hpp>
#include <hedron/generic_pd.hpp>
#include <hedron/hip.hpp>
#include <hedron/mtd.hpp>
#include <hedron/qpd.hpp>
#include <hedron/rights.hpp>
#include <hedron/sm.hpp>
#include <hedron/syscalls.hpp>
#include <hedron/typed_item.hpp>
#include <hedron/utcb.hpp>
#include <hedron/utcb_layout.hpp>

TEST(libhedron, libhedron_compiles_without_big_supernova_include)
{
    auto rights {hedron::mem_rights::full()};
    auto crd {hedron::crd {hedron::crd::type::MEM_CAP, rights, 0x1ul, 0x1ul}};
    ASSERT_EQ(crd.base(), 0x1ul);
}
