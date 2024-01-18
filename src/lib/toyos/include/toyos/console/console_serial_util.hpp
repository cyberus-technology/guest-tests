/* Copyright Cyberus Technology GmbH *
 *        All rights reserved        */

#pragma once

#include <toyos/acpi_tables.hpp>

uint16_t get_effective_serial_port(const std::string& serial_option, acpi_mcfg* mcfg);
