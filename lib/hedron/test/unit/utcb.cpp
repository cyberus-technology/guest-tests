/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <gtest/gtest.h>

#include <hedron/typed_item.hpp>
#include <hedron/utcb.hpp>

constexpr uint64_t DUMMY {0x13371337};

TEST(utcb, untyped_item_overflow_throws)
{
    std::unique_ptr<hedron::utcb> test_utcb {std::make_unique<hedron::utcb>()};
    memset(reinterpret_cast<void*>(test_utcb.get()), 0, sizeof(hedron::utcb));

    for (size_t i {0}; i < hedron::utcb::NUM_ITEMS; i++) {
        test_utcb->add_untyped_item(DUMMY);
    }

    ASSERT_ANY_THROW(test_utcb->add_untyped_item(DUMMY));
}

TEST(utcb, typed_item_overflow_throws)
{
    std::unique_ptr<hedron::utcb> test_utcb {std::make_unique<hedron::utcb>()};
    memset(reinterpret_cast<void*>(test_utcb.get()), 0, sizeof(hedron::utcb));

    auto typed {hedron::typed_item::translate_item(hedron::crd(0))};

    for (size_t i {0}; i < hedron::utcb::NUM_ITEMS / 2; i++) {
        test_utcb->add_typed_item(typed);
    }

    ASSERT_ANY_THROW(test_utcb->add_typed_item(typed));
}

TEST(utcb, item_collision_throws)
{
    std::unique_ptr<hedron::utcb> test_utcb {std::make_unique<hedron::utcb>()};
    memset(reinterpret_cast<void*>(test_utcb.get()), 0, sizeof(hedron::utcb));

    auto typed {hedron::typed_item::translate_item(hedron::crd(0))};

    for (size_t i {0}; i < hedron::utcb::NUM_ITEMS / 4; i++) {
        test_utcb->add_typed_item(typed);
    }

    for (size_t i {0}; i < hedron::utcb::NUM_ITEMS / 2; i++) {
        test_utcb->add_untyped_item(DUMMY);
    }

    ASSERT_ANY_THROW(test_utcb->add_typed_item(typed));
    ASSERT_ANY_THROW(test_utcb->add_untyped_item(DUMMY));
}
