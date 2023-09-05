/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <gtest/gtest.h>

#include <hedron/typed_item.hpp>

using namespace hedron;

static const uint64_t DUMMY {0x1337};

TEST(typed_item, crd_is_set_correctly)
{
    {
        crd c {DUMMY};
        auto item {typed_item::delegate_item(c, false, false, false, false)};
        ASSERT_EQ(DUMMY, item.get_value());
        ASSERT_EQ(c.value(), item.get_crd().value());
    }
    {
        crd c {DUMMY};
        auto item {typed_item::translate_item(c)};
        ASSERT_EQ(DUMMY, item.get_value());
        ASSERT_EQ(c.value(), item.get_crd().value());
    }
}

TEST(typed_item, flags_are_set_correctly)
{
    {
        crd c {0};
        auto item {typed_item::delegate_item(c, true, true, true, true)};
        ASSERT_EQ(0ul, item.get_value());
        ASSERT_EQ(typed_item::MAP_DMA | typed_item::MAP_GUEST | typed_item::HYPERVISOR | typed_item::DELEGATE_ITEM,
                  item.get_flags());
    }
    {
        crd c {0};
        auto item {typed_item::translate_item(c)};
        ASSERT_EQ(0ul, item.get_value());
        ASSERT_EQ(0ul, item.get_flags());
    }
    {
        crd c {0};
        auto item {typed_item::delegate_item(c, GUEST_DMA, true)};
        ASSERT_EQ(0ul, item.get_value());
        ASSERT_EQ(typed_item::MAP_DMA | typed_item::MAP_GUEST | typed_item::MAP_NOT_HOST | typed_item::HYPERVISOR
                      | typed_item::DELEGATE_ITEM,
                  item.get_flags());
    }
}
