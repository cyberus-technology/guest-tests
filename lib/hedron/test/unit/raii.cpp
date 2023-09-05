/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <gtest/gtest.h>

#include <hedron/generic_cap_range.hpp>

using namespace hedron;

namespace
{
struct no_revoke_policy
{
    static void revoke([[maybe_unused]] cap_sel sel, [[maybe_unused]] size_t num) {}
};

} // namespace

TEST(cap_range, simple_alloc_dealloc)
{
    // How often did we allocate and deallocate?
    static size_t alloc_called {0};
    static size_t dealloc_called {0};

    // What was allocated/deallocated?
    static size_t alloc_num {0};
    static size_t dealloc_num {0};
    static cap_sel dealloc_cap {0};

    // Test values
    static constexpr size_t test_cap_num {0x10};
    static const cap_sel test_cap_sel {0x1234};

    struct test_allocator
    {
        static cap_sel allocate(size_t num)
        {
            alloc_called++;
            alloc_num = num;
            return test_cap_sel;
        }
        static void free(cap_sel cap, size_t num)
        {
            dealloc_called++;

            dealloc_cap = cap;
            dealloc_num = num;
        }
    };

    // The actual test starts here.

    using test_cap_range = generic_cap_range<test_cap_num, test_allocator, no_revoke_policy>;

    {
        test_cap_range test_range;

        ASSERT_EQ(test_range.size(), test_cap_num);
        ASSERT_EQ(test_range.get(), test_cap_sel);
    }

    ASSERT_EQ(alloc_called, 1);
    ASSERT_EQ(dealloc_called, 1);

    ASSERT_EQ(alloc_num, test_cap_num);
    ASSERT_EQ(dealloc_num, test_cap_num);
    ASSERT_EQ(dealloc_cap, test_cap_sel);
}

TEST(cap_range, move_semantics)
{
    // How often did we allocate and deallocate?
    static size_t alloc_called {0};
    static size_t dealloc_called {0};

    // Test values
    static const cap_sel test_cap_sel {0x1234};

    struct test_allocator
    {
        static cap_sel allocate(size_t /*num*/)
        {
            alloc_called++;
            return test_cap_sel;
        }
        static void free(cap_sel /*cap*/, size_t /*num*/) { dealloc_called++; }
    };

    // The actual test starts here.

    using test_cap = generic_cap_range<1, test_allocator, no_revoke_policy>;

    {
        auto move_to_cap {test_cap::invalid()};

        ASSERT_EQ(alloc_called, 0);

        {
            test_cap move_from_cap;

            ASSERT_EQ(alloc_called, 1);

            move_to_cap = std::move(move_from_cap);

            // move_from_cap goes out of scope and is not freed.
        }

        ASSERT_EQ(dealloc_called, 0);

        // move_to_cap goes out of scope and is freed.
    }

    ASSERT_EQ(alloc_called, 1);
    ASSERT_EQ(dealloc_called, 1);
}

TEST(cap_range, revoke)
{
    static const cap_sel test_cap_sel {0x1234};

    struct test_allocator
    {
        static cap_sel allocate([[maybe_unused]] size_t num) { return test_cap_sel; }

        static void free([[maybe_unused]] cap_sel cap, [[maybe_unused]] size_t num) {}
    };

    // Store what we attempted to revoke.
    static cap_sel revoke_sel {0};
    static size_t revoke_num {0};

    struct test_revoke
    {
        static void revoke(cap_sel sel, size_t num)
        {
            revoke_sel = sel;
            revoke_num = num;
        }
    };

    // The actual test starts here.

    using test_cap = generic_cap_range<1, test_allocator, test_revoke>;

    {
        test_cap cap;
    }

    ASSERT_EQ(revoke_sel, test_cap_sel);
    ASSERT_EQ(revoke_num, 1);
}
