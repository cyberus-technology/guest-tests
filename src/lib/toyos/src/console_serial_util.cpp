/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#include <string>

#include <toyos/acpi_tables.hpp>
#include <toyos/console/console_serial.hpp>
#include <toyos/console/console_serial_util.hpp>

/**
 * Returns the effective serial port by parsing the corresponding cmdline
 * option.
 */
uint16_t get_effective_serial_port(const std::string& serial_option, acpi_mcfg* mcfg)
{
    constexpr std::string_view HEX_PREFIX = "0x";

    if (serial_option.empty()) {
        return discover_serial_port(mcfg);
    }

    /**
     * Checks if the serial option has a prefix following a number. This doesn't
     * verify the number.
     */
    auto has_prefix = [serial_option](const std::string_view& needle) {
        auto len_prefix_plus_number = serial_option.length() > needle.length() + 1;
        auto starts_with = serial_option.compare(0, needle.length(), needle) == 0;
        return len_prefix_plus_number && starts_with;
    };

    // TODO once we switch to a more modern libcxx, std::stoull should do this
    //  automatically. The current version just aborts when a prefix is found.
    if (has_prefix(HEX_PREFIX)) {
        auto number_str = serial_option.substr(HEX_PREFIX.length());
        return std::stoull(number_str, nullptr, 16);
    }
    else {
        return std::stoull(serial_option, nullptr, 10);
    }
}
