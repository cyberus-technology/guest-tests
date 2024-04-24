// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>

/**
 * Returns the boot command line.
 *
 * The returned option contains a value as soon as the cmdline was set by the
 * boot code.
 */
std::optional<std::string> get_boot_cmdline();
