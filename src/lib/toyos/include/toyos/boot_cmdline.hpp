/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <string>

/**
 * Returns the boot command line.
 *
 * The returned option contains a value as soon as the cmdline was set by the
 * boot code.
 */
std::optional<std::string> get_boot_cmdline();
