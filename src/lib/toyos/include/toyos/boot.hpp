/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <optional>
#include <string_view>

enum class boot_method
{
    MULTIBOOT1,
    MULTIBOOT2,
    XEN_PVH,
};

std::string_view boot_method_name(boot_method method);

/**
 * The chosen boot method.
 */
extern std::optional<boot_method> current_boot_method;
