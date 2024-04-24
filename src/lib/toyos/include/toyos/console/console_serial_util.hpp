// Copyright © 2024 Cyberus Technology GmbH <contact@cyberus-technology.de>
//
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <toyos/acpi_tables.hpp>

uint16_t get_effective_serial_port(const std::string& serial_option, acpi_mcfg* mcfg);
