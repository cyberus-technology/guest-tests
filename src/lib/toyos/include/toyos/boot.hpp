// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

/**
 * LOAD address specified in linker script.
 */
extern uint32_t LOAD_ADDR;

/**
* Returns the actual runtime load address.
*/
static uint32_t load_addr()
{
    // This is true as long as libtoyos is not relocatable in physical memory.
    return (uint32_t) reinterpret_cast<uint64_t>(&LOAD_ADDR);
}
