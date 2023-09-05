/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <algorithm>
#include <gtest/gtest.h>
#include <span.hpp>

TEST(span, span_should_iterate_correctly)
{
    int arr[] = {0, 1, 2, 3, 4};
    int num = arr[0];
    span<int> view(arr, 5);

    ASSERT_EQ(5, std::distance(view.begin(), view.end()));
    for (auto e : view) {
        ASSERT_EQ(num++, e);
    }

    span<int> empty {arr, 0};
    ASSERT_EQ(0, std::distance(empty.begin(), empty.end()));
}

TEST(span, span_allows_array_access)
{
    int arr[] {0, 1, 2, 3, 4};

    span<int> view {arr, 5};
    const span<int> const_view {arr, 5};

    for (int i = 0; i < static_cast<int>(const_view.size()); i++) {
        ASSERT_EQ(i, const_view[i]);
    }

    for (int i = 0; i < static_cast<int>(view.size()); i++) {
        ASSERT_EQ(i, view[i]);

        // Allows modification.
        view[i] = 0;
    }
}
